//
// Created by liuzikai on 11/2/19.
//

/**
 * Some terminology
 */

#ifndef _TASK_H
#define _TASK_H

#include "types.h"
#include "x86_desc.h"
#include "rtc.h"
#include "terminal.h"
#include "file_system.h"
#include "virtual_screen.h"

/** --------------- Process Control Block (PCB) --------------- */

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
#define TASK_FLAG_KERNEL_TASK    2U  // kernel thread (no user paging, no terminal)
#define TASK_WAITING_CHILD       4U  // waiting for child task to halt
#define TASK_WAITING_RTC         8U  // in waiting list of RTC
#define TASK_WAITING_TERMINAL    16U // in waiting list of terminal
#define TASK_IDLE_TASK           32U // idle thread (run only when no other task is runnable, and have minimal time)
#define TASK_TERMINAL_OWNER      64U // own terminal

struct task_t {
    uint8_t valid;  // 1 if current task_t is in use, 0 if not

    // Strings of executable name and arguments, stored in kernel stacks
    uint8_t* executable_name;
    uint8_t* args;

    uint32_t flags;  // flags for current task
    struct task_t* parent;

    uint32_t kesp;  // kernel stack ESP
    int32_t page_id;  // id for memory management

    task_list_node_t list_node;
    sched_control_t sched_ctrl;

    rtc_control_t rtc;

    uint8_t is_terminal_owner;
    terminal_t* terminal;


    file_array_t file_array;
};
typedef struct task_t task_t;


/** --------------- Process Kernel Memory (PKM) --------------- */

// Process kernel memory (PKM) consists of task control block and the kernel stack for a task
#define PKM_SIZE_IN_BYTES    8192
typedef union process_kernel_memory_t process_kernel_memory_t;
union process_kernel_memory_t {
    task_t pcb;
    uint8_t kstack[PKM_SIZE_IN_BYTES];  // stack for kernel status
};

#define PKM_STARTING_ADDR    0x800000  // PKM starts at 8MB (bottom of kernel image), going to low address
#define PKM_ALIGN_MASK     0xFFFFE000  // PKM is 8k-aligned, when in kernel_stack, mask ESP with this is current PCB


/** --------------- Task Managements --------------- */

#define TASK_MAX_COUNT    6  // maximum number of processes running at the same time
extern uint32_t task_count;

// TODO: implement these two pointer
task_t* running_task();
task_t* focus_task();

void running_task_start_waiting(task_list_node_t* task);
void task_terminal_read_done(task_list_node_t* task);

/** --------------- Interface for Pure Kernel State --------------- */

void task_init();
void task_run_initial_task();

/** --------------- System Calls Implementations --------------- */

int32_t system_execute(uint8_t *command, uint8_t wait_for_return, uint8_t new_terminal, int (*kernel_thread_eip)());
int32_t system_halt(int32_t status);
int32_t system_getargs(uint8_t *buf, int32_t nbytes);


/** --------------- Task List Related Helpers  --------------- */

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
 * Move a task to a new list
 * @param task        Pointer to the task to be moved
 * @param new_prev    Pointer to the new prev node of the task
 * @param new_next    Pointer to the new next node of the task
 * @note Will disable interrupt when performing work
 * @note Be VERY careful when using this function to move a task in the same list. Pointers of new_prev and new_next
 *       are still those BEFORE extracting the task.
 */
inline void move_task_to_list(task_t* task, task_list_node_t* new_prev, task_list_node_t* new_next);

/**
 * Move a task to the list after the given node
 * @param task    Pointer to the task to be moved
 * @param node    Pointer to the new prev node of the task
 * @note Will disable interrupt when performing work
 * @note Be VERY careful when using this function to move a task in the same list. Pointers of new_prev and new_next
 *       are still those BEFORE extracting the task.
 */

inline void move_task_after_node(task_t* task, task_list_node_t* node);

/**
 * Iterate through a task_list
 * @param var             variable name of current node
 * @param sentinel_node   sentinel node of a task list
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
