//
// Created by liuzikai on 11/2/19.
//

/**
 * See doc/task_and_sched.md
 */

#ifndef _TASK_H
#define _TASK_H

#include "types.h"
#include "x86_desc.h"

#include "rtc.h"
#include "terminal.h"
#include "file_system.h"
#include "signal.h"

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
    signal_struct_t signals;
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
