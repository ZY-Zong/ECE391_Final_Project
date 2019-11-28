//
// Created by liuzikai on 11/2/19.
//

#include "task.h"
#include "lib.h"
#include "file_system.h"
#include "task_paging.h"
#include "task_sched.h"

#define TASK_ENABLE_CHECKPOINT    0
#if TASK_ENABLE_CHECKPOINT
#include "tests.h"
#endif

#define task_slot(idx) ((task_t *) (PKM_STARTING_ADDR - (idx + 1) * PKM_SIZE_IN_BYTES))  // address of idx-th PCB
uint32_t task_count = 0;  // count of tasks that has started

#define USER_STACK_STARTING_ADDR  (0x8400000 - 1)  // User stack starts at 132MB - 1 (with paging enabled)

// Wait list of tasks that are waiting for child to halt
task_list_node_t wait4child_list = TASK_LIST_SENTINEL(wait4child_list);

task_t *terminal_user_task[TERMINAL_MAX_COUNT];
task_t *focus_task_ = NULL;

/** ================ Function Declarations =============== */

static task_t *task_deallocate(task_t *task);

static void init_thread_main();

/**
 * This macro yield CPU from current task (_prev_) to new task (_next_) and return after _next_ terminate
 * @param kesp_save_to    Save ESP of kernel stack of _prev_ to this address
 * @param new_esp         Starting ESP of _next_
 * @param new_eip         Starting EIP of _next_
 * @param ret             After _next_ terminate, return value is written to this address, and this macro returns
 * @note Make sure paging of _next_ is all set
 * @note Make sure TSS is set to kernel stack of _next_
 * @note After switching, the top of _prev_ stack is the return address (label 1)
 * @note To switch back, load the return value in EAX, switch stack to _prev_, and run `ret` on _prev_ stack
 */
#define execute_launch(kesp_save_to, new_esp, new_eip, ret) asm volatile ("                    \
    pushfl          /* save flags on the stack */                                   \n\
    pushl %%ebp     /* save EBP on the stack */                                     \n\
    pushl $1f       /* return address to label 1, on top of the stack after iret */ \n\
    movl %%esp, %0  /* save current ESP */                                          \n\
    /* The following stack linkage is for IRET */                                   \n\
    pushl $0x002B   /* user SS - USER_DS */                                         \n\
    pushl %2        /* user ESP */                                                  \n\
    pushf           /* flags (new program should not care) */                       \n\
    pushl $0x0023   /* user CS - USER_CS  */                                        \n\
    pushl %3        /* user EIP */                                                  \n\
    iret            /* enter user program */                                        \n\
1:  popl %%ebp      /* restore EBP, must before following instructions */           \n\
    movl %%eax, %1  /* return value pass by halt() in EAX */                        \n\
    popfl           /* restore flags */"                                              \
    : "=m" (kesp_save_to), /* must write to memory, or halt() will not get it */      \
      "=m" (ret)                                                                      \
    : "rm" (new_esp), "rm" (new_eip)                                                  \
    : "cc", "memory"                                                                  \
)


/**
 * This jump back to label 1 in execute_launch
 * @param old_esp    ESP of parent task. On the top of parent's stack should be return address to parent's code
 * @param ret        Return value of current thread. Will be stored to EAX
 * @note Make sure paging of the destination task is all set
 * @note Make sure TSS is set to kernel stack of the destination task
 */
#define halt_backtrack(old_esp, ret) asm volatile ("                                        \
    movl %0, %%esp  /* load back old ESP */                                      \n\
    /* EAX store the return status */                                            \n\
    ret  /* on the old kernel stack, return address is on the top of the stack */" \
    :                                                                              \
    : "r" (old_esp)   , "a" (ret)                                                  \
    : "cc", "memory"                                                               \
)

/**
 * Get current task based on ESP. Only for usage in kernel state.
 * @return Pointer to current task
 */
task_t *running_task() {
    task_t *ret;
    asm volatile ("movl %%esp, %0  \n\
                   andl $0xFFFFE000, %0    /* PKM_ALIGN_MASK */" \
                   : "=r" (ret) \
                   : \
                   : "cc" \
                   );
    return ret;
}

/**
 * Initialize task management
 */
void task_init() {
    int i;

    // Initialize task slots
    for (i = 0; i < TASK_MAX_COUNT; i++) {
        task_slot(i)->valid = 0;
    }
    task_count = 0;

    // Initialize terminal user list
    for (i = 0; i < TERMINAL_MAX_COUNT; i++) {
        terminal_user_task[i] = NULL;
    }

    // Initialize paging related things
    task_paging_init();

    // Initialize scheduler
    sched_init();
}

/**
 * Run init thread
 */
void task_run_initial_task() {
    system_execute((uint8_t *) "initd", 0, 0, init_thread_main);
    // Should never return
    DEBUG_ERR("task_run_initial_task(): init thread should never return!");
}

/**
 * Find an available slot in task list, mark as valid and return. If no available, return NULL
 * @return Pointer to newly allocated task_t, or NULL is no available
 */
static task_t *task_allocate_new_slot() {
    int i;

    if (task_count >= TASK_MAX_COUNT) return NULL;

    task_count++;
    for (i = 0; i < TASK_MAX_COUNT; i++) {
        if (!task_slot(i)->valid) {
            task_slot(i)->valid = 1;
            return task_slot(i);
        }
    }

    DEBUG_ERR("task_count is inconsistent");
    return NULL;
}

/**
 * Deallocate a task from task slot
 * @param task    Pointer to task_t of the task to be removed
 * @return Pointer to its parent task
 */
static task_t *task_deallocate(task_t *task) {
    task_t *ret = task->parent;
    task->valid = 0;
    task_count--;
    return ret;
}

/**
 * Helper function to parse command into executable name and argument string
 * @param command    [In] string to be parse. [Out] executable name
 * @param args       [Out] pointer to string of arguments
 * @return 0 if success
 */
static int32_t execute_parse_command(uint8_t *command, uint8_t **args) {
    while (*command != '\0') {
        if (*command == ' ') {
            *command = '\0';  // split executable name and args;
            *args = command + 1;  // output args string
            return 0;
        }
        command++;  // move to next character
    }
    // No args
    *args = NULL;
    return 0;
}

/**
 * Actual implementation of execute() system call
 * @param command    Command to be executed
 * @param wait_for_return    If set to 1, this function will return after new program halt() with its halt state.
 *                           If set to 0, this function will return -1
 *                           Must be 0 for init task, which will never return
 * @param new_terminal       If set to 1, a new terminal will be assigned to the task.
 *                           If set to 0, the new task will share terminal with running_task
 * @param kernel_thread_eip  If set to NULL, this function will execute user program by arg command.
 *                           If set to a function, a kernel thread will be created (no paging, no terminal, should never
 *                           return, but can halt) and command will only be used as strings in PCB
 * @return Terminate status of the program (0-255 if program terminate by calling halt(), 256 if exception occurs)
 * @note New program given in command will run immediately, and this function will return after its terminate
 */
int32_t system_execute(uint8_t *command, uint8_t wait_for_return, uint8_t new_terminal, void (*kernel_thread_eip)()) {

    task_t *task;
    uint32_t start_eip;
    int32_t program_ret;
    uint32_t temp;

    /** --------------- Phase 1. Setup kernel data structure of new task --------------- */

    // Allocate a new PCB
    if (NULL == (task = task_allocate_new_slot())) return -1;  // no available slot


    // Setup flags and parent. Will be used in the following code
    task->flags = 0;
    if (task_count == 1) {  // this is the initial task
        task->flags |= TASK_INIT_TASK;
        task->parent = NULL;
        if (wait_for_return) {
            DEBUG_ERR("system_execute(): init task should not wait for return");
            task_deallocate(task);
            return -1;
        }
    } else {
        task->parent = running_task();
    }
    if (kernel_thread_eip != 0) {
        task->flags |= TASK_FLAG_KERNEL_TASK;
        if (wait_for_return) {
            DEBUG_ERR("system_execute(): kernel task should not wait for return");
            task_deallocate(task);
            return -1;
        }
    }

    // Initialize kernel ESP to the bottom of PKM
    task->kesp = ((uint32_t) task) + PKM_SIZE_IN_BYTES - 1;

    // Parse executable name and arguments
    if (execute_parse_command(command, &task->args) != 0) {
        DEBUG_ERR("system_execute(): fail to parse executable and arguments");
        task_deallocate(task);
        return -1;  // invalid command
    }
    task->executable_name = command;

    // Store the executable name and argument string to the kernel stack of new program, or they will be inaccessible
    temp = strlen((int8_t *) task->executable_name);
    task->kesp -= temp;
    task->executable_name = (uint8_t *) strcpy((int8_t *) task->kesp, (int8_t *) task->executable_name);

    if (task->args != NULL) {
        temp = strlen((int8_t *) task->args);
        task->kesp -= temp;
        task->args = (uint8_t *) strcpy((int8_t *) task->kesp, (int8_t *) task->args);
    }

    // Init RTC control info
    rtc_control_init(&task->rtc);

    // Assign terminal
    if (task->flags & TASK_FLAG_KERNEL_TASK) {
        if (new_terminal) {
            DEBUG_ERR("system_execute(): kernel thread can't have terminal");
            task_deallocate(task);
            return -1;
        }
        task->terminal = NULL;
    } else {
        if (new_terminal) {
            task->flags |= TASK_TERMINAL_OWNER;
            task->terminal = terminal_allocate();
            terminal_user_task[task->terminal->terminal_id] = task;
            if (task->terminal == NULL) {
                DEBUG_ERR("system_execute(): fail to allocate new terminal");
                task_deallocate(task);
                return -1;
            }
            if (terminal_vid_open(task->terminal->terminal_id) != 0) {
                DEBUG_ERR("system_execute(): fail to allocate video memory new terminal");
                terminal_deallocate(task->terminal);
                task_deallocate(task);
                return -1;
            }
        } else {
            if (task->flags & TASK_INIT_TASK) {
                task->terminal = NULL;
            } else {
                task->terminal = running_task()->terminal;  // inherent terminal from caller
                if (wait_for_return) {  // new task will be the actual user of the terminal until it halt
                    terminal_user_task[task->terminal->terminal_id] = task;
                }
            }
        }
    }

    // Setup some initial value of task list, in order for sched_insert_to_head() to work correctly.
    task->list_node.prev = task->list_node.next = &task->list_node;

    // Init opened file list
    init_file_array(&task->file_array);

    if (task->flags & TASK_FLAG_KERNEL_TASK) {
        // Setup paging, run program loader, get new EIP
        // NOTICE: after setting up paging for new program, arg command become useless
        if ((task->page_id = task_set_up_memory(command, &start_eip, task->terminal->terminal_id)) < 0) {
            task_deallocate(task);
            return -1;
        }
    } else {
        task->page_id = -1;
        start_eip = (uint32_t) kernel_thread_eip;
        // TODO: test whether execute_launch() work for kernel function
    }

    /** --------------- Phase 2. Ready to go. Setup scheduler --------------- */

    // Put current task into list for parents that wait for child to return
    if (wait_for_return) {
        // Safe to use running_task() since init task should not wait for return, which has been checked above
        running_task()->flags |= TASK_WAITING_CHILD;
        sched_move_running_after_node(&wait4child_list);
    }

    // Put child task into run queue
    sched_refill_time(task);
    sched_insert_to_head(task);
    // Don't use launch() function of sched, perform context switch manually as follows

    /** --------------- Phase 3. Very ready to go. Do assembly level switch --------------- */
    // Set tss to new task's kernel stack to make sure system calls use correct stack
    tss.esp0 = task->kesp;

    // Jump to user program entry
    if (task->flags & TASK_INIT_TASK) {
        execute_launch(temp, USER_STACK_STARTING_ADDR, start_eip, program_ret);
    } else {
        execute_launch(running_task()->kesp, USER_STACK_STARTING_ADDR, start_eip, program_ret);
    }

    // The child task running...

    /** --------------- Phase 4. Ready to go. Insert the new task into schedule --------------- */

    // Sanity check. TASK_WAITING_CHILD should be cleared when this thread is re-activated
    if (running_task()->flags & TASK_WAITING_CHILD) {
        DEBUG_ERR("task of %s is still waiting for exit of child and should not be waken up!",
                  running_task()->executable_name);
    }

#if TASK_ENABLE_CHECKPOINT
    checkpoint_task_paging_consistent();  // check paging is recovered to current task
#endif

    return program_ret;
}

/**
 * Actual implementation of halt() system call
 * @param status    Exit code of current task (size are enlarged to support 256 return from exception)
 * @return This function should never return
 */
int32_t system_halt(int32_t status) {

    task_list_node_t temp_list = TASK_LIST_SENTINEL(temp_list);
    task_t *task = running_task();  // the task to halt
    task_t *parent = task->parent;

    if (task_count == 0) {   // pure kernel state
        DEBUG_ERR("Can't halt pure kernel state!");
        return -1;
    }

    if (task->flags & TASK_INIT_TASK) {
        printf("Init task halt with status %d. OS halt.", status);
        while (1) {}
    }

    /** --------------- Phase 1. Remove current task from scheduler --------------- */

    // Remove task from run queue
    sched_move_running_after_node(&temp_list);

    if (parent) {
        // Re-activate parent
        parent->flags &= ~TASK_WAITING_CHILD;
        sched_refill_time(parent);
        sched_insert_to_head(parent);
    }

    /** --------------- Phase 2. Tear down kernel data structure --------------- */

#if TASK_ENABLE_CHECKPOINT
    checkpoint_task_paging_consistent();  // check paging is consistent
#endif

    // Deallocate file array
    clear_file_array(&task->file_array);

#if TASK_ENABLE_CHECKPOINT
    checkpoint_task_closed_all_files();
#endif

    // Deallocate terminal
    if (task->flags & TASK_TERMINAL_OWNER) {
        terminal_user_task[task->terminal->terminal_id] = NULL;
        terminal_vid_close(task->terminal->terminal_id);  // deallocate terminal video memory
        terminal_deallocate(task->terminal);  // deallocate terminal control block
    } else {
        if (parent->terminal->terminal_id != task->terminal->terminal_id) {
            DEBUG_ERR("system_halt(): task uses terminal %d but not own it, but its parent uses terminal %d",
                      task->terminal->terminal_id, parent->terminal->terminal_id);
        }
        terminal_user_task[parent->terminal->terminal_id] = parent;
    }

    // Deallocate task
    task_deallocate(task);

    /** --------------- Phase 3. Ready to go. Do low level switch --------------- */

    task_reset_paging(task->page_id, parent->page_id);  // switch page to parent

    tss.esp0 = parent->kesp;  // set tss to parent's kernel stack to make sure system calls use correct stack

    halt_backtrack(parent->kesp, status);

    // Goodbye world...

    DEBUG_ERR("halt(): should never return");

    return -1;
}

/**
 * Actual implementation of getargs() system call
 * @param buf       String buffer to accept args
 * @param nbytes    Maximal number of bytes to write to buf
 * @return 0 on success, -1 on no argument or argument string can't fit in nbytes
 */
int32_t system_getargs(uint8_t *buf, int32_t nbytes) {
    uint8_t *args = running_task()->args;
    if (args == NULL) return -1;  // no args
    if (strlen((int8_t *) args) >= nbytes) return -1;  // can not fit into buf (including ending NULL char)
    strncpy((int8_t *) buf, (int8_t *) args, nbytes);
    return 0;
}

inline void move_task_to_list_unsafe(task_t *task, task_list_node_t *new_prev, task_list_node_t *new_next) {
    task_list_node_t *n = &task->list_node;
    n->next->prev = n->prev;
    n->prev->next = n->next;
    n->prev = new_prev;
    new_prev->next = n;
    n->next = new_next;
    new_next->prev = n;
}

inline void move_task_after_node_unsafe(task_t *task, task_list_node_t *node) {
    move_task_to_list_unsafe(task, node, node->next);
}

static void init_thread_main() {
    int32_t ret;

    system_execute((uint8_t *) "shell", 0, 1, NULL);
    system_execute((uint8_t *) "shell", 0, 1, NULL);
    while (1) {
        ret = system_execute((uint8_t *) "shell", 1, 1, NULL);
        printf("Last shell has halted with status %d. Restarting...", ret);
    }

    system_halt(0);
}

task_t *focus_task() {
#if TASK_ENABLE_CHECKPOINT
    if (focus_task_ == NULL) {
        DEBUG_ERR("focus_task is NULL!");
    }
#endif
    return focus_task_;
}

void task_change_focus(int32_t terminal_id) {
    if (terminal_id < 0 || terminal_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("task_change_focus(): invalid terminal_id");
        return;
    }

    uint32_t flags;

    cli_and_save(flags);
    {

        focus_task_->terminal->screen_x = screen_x;
        focus_task_->terminal->screen_y = screen_y;

        terminal_active_vid_switch(terminal_id, focus_task_->terminal->terminal_id);
        focus_task_ = terminal_user_task[terminal_id];

        screen_x = focus_task_->terminal->screen_x;
        screen_y = focus_task_->terminal->screen_y;

    }
    restore_flags(flags);
}