//
// Created by liuzikai on 11/2/19.
//

/**
 * PCB and PKM
 * We use term 'task' for process in AuroraOS. Each task will has a Process Control Block (PCB) that store info for this
 *  specific task, which is task_t.
 * Each task has a 8KB Process Kernel Memory (PKM), growing from 8MB (the end of kernel image in memory) to lower
 *  address, which can be accessed through task_slot(idx) in task.c. The PCB lies on the top (low address) of PKM, and
 *  kernel stack (stack for kernel state such as interrupts and system calls when in this task) grows from bottom (high
 *  address). Notice that if TASK_MAX_COUNT is set too large, PKMs may overlap with kernel image but no check is
 *  performed.
 *
 * RUNNING_TASK AND FOCUS_TASK
 * running_task() is the pointer to current running task (which interrupt happens among, or caller of system call).
 *  Since PKMs are 8KB-aligned, we can get running_task() by align current ESP to 8KB. All system calls should use
 *  this as caller task, and screen output should be write to corresponding video memory of this.)
 * focus_task() is the task that is at the foreground and accept keyboard input. There is a local variable that maintain
 *  this variable and it is changed through task_change_focus(). Although terminal can be shared (for example, a shell
 *  execute cat, they share a terminal so that all content is written on the same screen,) we assume that only one
 *  task can be the actual user of a terminal that accept keyboard input (for example, a shell execute a program and
 *  wait for its halt, then the new program become the actual user; if the shell won't wait for its halt, it runs
 *  as background task and the shell remains terminal user.) The mapping from terminal ID to task is maintained in the
 *  terminal_user_task[] in task.c.
 *
 * EXECUTE AND HALT
 * system_execute() is the extended version of system call execute().
 * system_halt() is also extended so that it can accept status greater than 255 (use when halt because of exceptions)
 * The most important low-level context switch functions are execute_launch() and halt_backtrack() in task.c. See
 *  comments there.
 * It's ok (maybe necessary to use lock in execute() and halt()). Flags for new program are fixed value that enable
 *  IF.
 *
 * TASK LIST
 * Without scheduling, the implementation of terminal and RTC read() is to run an infinity loop and wait for interrupts
 *  to update status. But with scheduling, it's a waste of time if the available time for a task is used to run
 *  infinity loop. So we implement wait lists. When a task (mostly running_task(), which is the caller of system calls)
 *  is put into waiting status, it is removed from run queue and put into a corresponding wait list. When it's time
 *  for it to return, it is inserted back to the HEAD of run queue, which make terminal more responsive and RTC more
 *  accurate.
 * Notice that both wait lists and run queue can be implemented with doubly-linked list. Also, a task can only be in
 *  one list at a time, no matter it's run queue or a wait list. So we implement general task list (with sentinel as
 *  list head). task_list_node_t is the structure for doubly-linked list node in each task_t. task_from_node() uses
 *  tricks of address arithmetic to get the task_t that contains the node (inspired by linux 2.6 source, but
 *  simplified.) Several helper functions are provided. MAKE SURE READ COMMENTS BEFORE USING THEM.
 * For scheduling, see comments in task_sched.h.
 */

#ifndef _TASK_H
#define _TASK_H

#include "types.h"
#include "x86_desc.h"

#include "rtc.h"
#include "terminal.h"
#include "file_system.h"

/** ============== Process Control Block (PCB) ============== */

struct task_list_node_t {
    struct task_list_node_t* next;
    struct task_list_node_t* prev;
};
typedef struct task_list_node_t task_list_node_t;

struct sched_control_t {
    int32_t remain_time;
};
typedef struct sched_control_t sched_control_t;


#define TASK_INIT_TASK           1U  // initial process
#define TASK_KERNEL_TASK    2U  // kernel thread (no user paging, no terminal, should not return but can halt)
#define TASK_WAITING_CHILD       4U  // waiting for child task to halt
#define TASK_WAITING_RTC         8U  // in waiting list of RTC
#define TASK_WAITING_TERMINAL    16U // in waiting list of terminal
#define TASK_TERMINAL_OWNER      32U // own terminal
#define TASK_IDLE_TASK           64U // idle task (must be kernel task, only run when no other runnable task)

typedef struct task_t task_t;
struct task_t {
    uint8_t valid;  // 1 if current task_t is in use, 0 if not

    // Strings of executable name and arguments, stored in kernel stacks
    uint8_t* executable_name;
    uint8_t* args;

    uint32_t flags;  // flags for current task
    task_t* parent;

    uint32_t kesp_base;      // kernel stack base (above the executable name and the args), for TSS
    uint32_t kesp;           // kernel stack pointer. Not equal to ESP for running_task. Updated at context switch
    int32_t page_id;         // id for memory management
    uint8_t vidmap_enabled;  // the task has map video memory to user space with vidmap() system call

    task_list_node_t list_node;
    sched_control_t sched_ctrl;

    rtc_control_t rtc;

    terminal_t* terminal;  // if no terminal, use &null_terminal

    file_array_t file_array;
};


/** ============== Process Kernel Memory (PKM) ============== */

// Process kernel memory (PKM) consists of task control block and the kernel stack for a task
#define PKM_SIZE_IN_BYTES    8192
typedef union process_kernel_memory_t process_kernel_memory_t;
union process_kernel_memory_t {
    task_t pcb;
    uint8_t kstack[PKM_SIZE_IN_BYTES];  // stack for kernel status
};

#define PKM_STARTING_ADDR    0x800000  // PKM starts at 8MB (bottom of kernel image), going to low address
#define PKM_ALIGN_MASK     0xFFFFE000  // PKM is 8k-aligned, when in kernel_stack, mask ESP with this is current PCB


/** ============== Task Managements ============== */

#define TASK_MAX_COUNT    8  // maximum number of processes running at the same time
extern uint32_t task_count;

task_t* running_task();
task_t* focus_task();

void task_change_focus(int32_t terminal_id);
void task_apply_user_vidmap(task_t* task);

/** ============== Interface for Pure Kernel State ============== */

void task_init();
void task_run_initial_task();

/** ============== System Calls Implementations ============== */

int32_t system_execute(uint8_t *command, int8_t wait_for_return, uint8_t new_terminal, void (*kernel_task_eip)());
int32_t system_halt(int32_t status);
int32_t system_getargs(uint8_t *buf, int32_t nbytes);


/** ============== Task List Related Helpers  ============== */

/**
 * Initialization value of sentinel node of a task list
 * @param node    Name of the sentinel
 */
#define TASK_LIST_SENTINEL(node)    {&(node), &(node)}

/**
 * Get pointer to task_t that contain given task list node
 * @param ptr_node    Pointer to a task list node
 * @return Pointer to the task_t that contain the task list node
 * @note Not for sentinel node
 */
#define task_from_node(ptr_node)    ((task_t *) (((char *) (ptr_node)) - __builtin_offsetof(task_t, list_node)))

/**
 * Move a task to a new list (lock needed)
 * @param task        Pointer to the task to be moved
 * @param new_prev    Pointer to the new prev node of the task
 * @param new_next    Pointer to the new next node of the task
 * @note Use lock OUTSIDE as you need, since pointers are stored on the calling stack and won't get changed if
 *       interrupts happens between
 * @note Be VERY careful when using this function to move a task in the same list. Pointers of new_prev and new_next
 *       are still those BEFORE extracting the task.
 */
inline void move_task_to_list_unsafe(task_t* task, task_list_node_t* new_prev, task_list_node_t* new_next);

/**
 * Move a task to the list after the given node (lock needed)
 * @param task    Pointer to the task to be moved
 * @param node    Pointer to the new prev node of the task
 * @note Use lock OUTSIDE as you need, since pointers are stored on the calling stack and won't get changed if
 *       interrupts happens between
 * @note Be VERY careful when using this function to move a task in the same list. Pointers of new_prev and new_next
 *       are still those BEFORE extracting the task.
 */
inline void move_task_after_node_unsafe(task_t* task, task_list_node_t* node);

/**
 * Iterate through a task_list
 * @param var             Variable name of current node
 * @param sentinel_node   Sentinel node of a task list
 * @note Not safe for removal
 */
#define task_list_for_each(var, sentinel_node) \
	for (var = (sentinel_node)->next; var != (sentinel_node); var = var->next)

/**
 * Iterate through a task_list against removal
 * @param var             variable name of pointer to current task list node
 * @param sentinel_node   sentinel node of a task list
 * @param temp            temp variable name of type task_list_node_t*
 */
#define task_list_for_each_safe(var, sentinel_node, temp)     \
	for (var = (sentinel_node)->next, temp = var->next; \
	     var != (sentinel_node);                            \
	     var = temp, temp = var->next)

#endif // _TASK_H
