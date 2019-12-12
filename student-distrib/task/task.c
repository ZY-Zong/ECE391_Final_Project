//
// Created by liuzikai on 11/2/19.
//

#include "task.h"
#include "../lib.h"
#include "../file_system.h"
#include "task_paging.h"
#include "task_sched.h"
#include "../vidmem.h"
#include "../signal.h"

#define TASK_ENABLE_CHECKPOINT    0
#if TASK_ENABLE_CHECKPOINT
#include "tests.h"
#endif

#define task_slot(idx) ((task_t *) (PKM_STARTING_ADDR - (idx + 1) * PKM_SIZE_IN_BYTES))  // address of idx-th PCB
volatile uint32_t task_count = 0;  // count of tasks that has started

#define USER_STACK_STARTING_ADDR  (0x8400000 - 1)  // User stack starts at 132MB - 1 (with paging enabled)

// Wait list of tasks that are waiting for child to halt
task_list_node_t wait4child_list = TASK_LIST_SENTINEL(wait4child_list);

task_t *terminal_fg_task[TERMINAL_MAX_COUNT];  // terminal foreground task
task_t *focus_task_ = NULL;

/** ================ Function Declarations =============== */

static task_t *task_deallocate(task_t *task);

static void init_task_main();

static void idle_task_main();

static void task_set_focus_task(task_t *task);

static int get_another_term_id(int term_id_start);

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
    pushl $0x206    /* flags (new program should not care, but IF = 1) */           \n\
    pushl $0x0023   /* user CS - USER_CS  */                                        \n\
    pushl %3        /* user EIP */                                                  \n\
    iret            /* enter user program */                                        \n\
1:  popl %%ebp      /* restore EBP, must before following instructions */           \n\
    movl %%eax, %1  /* return value pass by halt() in EAX */                        \n\
    popfl           /* restore flags */"                                              \
    : "=m" (kesp_save_to), /* must write to memory, or halt() will not get it */      \
      "=m" (ret)                                                                      \
    : "r" (new_esp), "r" (new_eip) /* must be passed in registers ESP changes     */  \
    : "cc", "memory"                                                                  \
)

/**
 * This macro yield CPU from current task (_prev_) to new KERNEL task (_next_) and return after _next_ terminate
 * @param kesp_save_to    Save ESP of kernel stack of _prev_ to this address
 * @param new_esp         Starting ESP of _next_
 * @param new_eip         Starting EIP of _next_
 * @param ret             After _next_ terminate, return value is written to this address, and this macro returns
 * @note Make sure paging of _next_ is all set
 * @note Make sure TSS is set to kernel stack of _next_
 * @note After switching, the top of _prev_ stack is the return address (label 1)
 * @note To switch back, load the return value in EAX, switch stack to _prev_, and run `ret` on _prev_ stack
 */
#define execute_launch_in_kernel(kesp_save_to, new_esp, new_eip, ret) asm volatile (" \
    pushfl          /* save flags on the stack */                                   \n\
    pushl %%ebp     /* save EBP on the stack */                                     \n\
    pushl $1f       /* return address to label 1, on top of the stack after iret */ \n\
    movl %%esp, %0  /* save current ESP */                                          \n\
    movl %2, %%esp  /* set new ESP */                                               \n\
    pushl %3        /* new EIP */                                                   \n\
    pushl $0x206    /* flags (new program should not care, but IF = 1) */           \n\
    popfl                                                                           \n\
    ret             /* enter new kernel task */                                     \n\
1:  popl %%ebp      /* restore EBP, must before following instructions */           \n\
    movl %%eax, %1  /* return value pass by halt() in EAX */                        \n\
    popfl           /* restore flags */"                                              \
    : "=m" (kesp_save_to), /* must write to memory, or halt() will not get it */      \
      "=m" (ret)                                                                      \
    : "r" (new_esp), "r" (new_eip)                                                    \
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
        terminal_fg_task[i] = NULL;
    }

    // Initialize paging related things
    task_paging_init();

    // Initialize scheduler
    sched_init();
}

/**
 * Run init thread
 * @note When this function is called, IF flag should be 1, otherwise idle task won't be able to yield processor
 *       to other task
 */

void task_run_initial_task() {
    uint32_t flags;
    cli_and_save(flags);
    {
        system_execute((uint8_t *) "initd", 0, 0, init_task_main);
        // Should never return
        DEBUG_ERR("task_run_initial_task(): init thread should never return!");
    }
    restore_flags(flags);
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
 *                           If set to 0, this function will return -1 after caller task is reactivated
 *                           Must be 0 for init task, which will never return
 *                           If set to -1, an ideal task will be created. Must be kernel task. This funtion return
 *                             -1 after caller task is reactivated
 * @param new_terminal       If set to 1, a new terminal will be assigned to the task, and it will become the new
 *                             focus task
 *                           If set to 0, the new task will share terminal with running_task
 * @param kernel_task_eip    If set to NULL, this function will execute user program by arg command.
 *                           If set to a function, a kernel thread will be created (no paging, no terminal, should never
 *                             return, but can halt) and command will only be used as strings in PCB
 * @return Terminate status of the program (0-255 if program terminate by calling halt(), 256 if exception occurs)
 * @note New program given in command will run immediately
 * @note Put this function into a lock
 */
int32_t system_execute(uint8_t *command, int8_t wait_for_return, uint8_t new_terminal, void (*kernel_task_eip)()) {

    /**
     * Note: be careful of use of running_task() since it's invalid for init task
     */

    task_t *task;
    uint32_t start_eip;
    int32_t program_ret;
    uint32_t temp;

    /** --------------- Phase 1. Setup kernel data structure of new task --------------- */

    // Allocate a new PCB
    if (NULL == (task = task_allocate_new_slot())) return -1;  // no available slot
    // Clean up #1 starts: task_deallocate(task)


    // Setup flags and parent. Will be used in the following code.
    task->flags = 0;

    if (task_count == 1) {  // init task
        task->flags |= TASK_INIT_TASK;
        task->parent = NULL;
        if (wait_for_return == 1) {
            DEBUG_ERR("system_execute(): init task should not wait for return");
            task_deallocate(task);
            return -1;
        }
    } else {
        if (wait_for_return == 1) {
            task->parent = running_task();
        } else {
            task->parent = NULL;  // do not wake up running_task() when halt
        }
        if (wait_for_return == 0 && new_terminal == 0 && running_task()->terminal->terminal_id != NULL_TERMINAL_ID) {
            DEBUG_ERR(
                    "system_execute(): new task will inherit non-NULL terminal from running task, so parent must wait.");
            return -1;
        }
    }
    if (kernel_task_eip != NULL) {  // kernel task
        if (wait_for_return == 1) {
            DEBUG_ERR("system_execute(): kernel task should not wait for return");
            task_deallocate(task);
            return -1;
        }
        if (new_terminal) {
            DEBUG_ERR("system_execute(): kernel thread can't have terminal");
            task_deallocate(task);
            return -1;
        }
        task->flags |= TASK_KERNEL_TASK;
    }
    if (wait_for_return == -1) {  // idle task
        if (kernel_task_eip == NULL) {
            DEBUG_ERR("system_execute(): idle task must be kernel task");
            task_deallocate(task);
            return -1;
        }
        task->flags |= TASK_IDLE_TASK;
    }

    task->vidmap_enabled = 0;

    // Initialize kernel ESP to the bottom of PKM. Must before copying executable_name and args
    task->kesp_base = ((uint32_t) task) + PKM_SIZE_IN_BYTES - 1;

    // Parse executable name and arguments
    if (execute_parse_command(command, &task->args) != 0) {
        DEBUG_ERR("system_execute(): fail to parse executable and arguments");
        task_deallocate(task);
        return -1;  // invalid command
    }
    task->executable_name = command;

    // Store the executable name and argument string to the kernel stack of new program, or they will be inaccessible
    // Must be after setting up kesp_base
    temp = strlen((int8_t *) task->executable_name);
    task->kesp_base -= temp;
    task->executable_name = (uint8_t *) strcpy((int8_t *) task->kesp_base, (int8_t *) task->executable_name);

    if (task->args != NULL) {
        temp = strlen((int8_t *) task->args);
        task->kesp_base -= temp;
        task->args = (uint8_t *) strcpy((int8_t *) task->kesp_base, (int8_t *) task->args);
    }

    task->kesp = task->kesp_base;  // empty kernel stack

    // Setup some initial value of task list, in order for sched_insert_to_head_unsafe() to work correctly.
    task->list_node.prev = task->list_node.next = &(task->list_node);

    /** --------------- Phase 2. Load components and set up memory --------------- */

    if (task->flags & TASK_KERNEL_TASK) {
        task->terminal = &null_terminal;  // kernel task must not have terminal, which has been checked above
        task->page_id = -1;  // kernel task has no user paging
        start_eip = (uint32_t) kernel_task_eip;  // kernel task starts at given address
    } else {
        if (new_terminal) {  // allocate a new terminal
            task->flags |= TASK_TERMINAL_OWNER;

            // Allocate terminal control structure
            if ((task->terminal = terminal_allocate()) == NULL) {
                DEBUG_ERR("system_execute(): fail to allocate new terminal");
                task_deallocate(task);
                return -1;
            }
            // Clean up #2 starts: terminal_deallocate(task->terminal)

            // Allocate video memory
            if (terminal_vidmem_open(task->terminal->terminal_id, &task->terminal->screen_char) != 0) {
                DEBUG_ERR("system_execute(): fail to allocate video memory new terminal");
                terminal_deallocate(task->terminal);
                task_deallocate(task);
                return -1;
            }
            // Clean up #3: terminal_vidmem_close(task->terminal->terminal_id)

            // Create new window
            if (gui_new_window(&task->terminal->win, task->terminal->screen_char) != 0) {
                DEBUG_ERR("system_execute(): fail to create new window");
                terminal_vidmem_close(task->terminal->terminal_id);
                terminal_deallocate(task->terminal);
                task_deallocate(task);
            }
            // Clean up #4: gui_destroy_window(&task->terminal->win)

        } else {  // inherit terminal from its parent or no terminal if it's the init task
            if (task->flags & TASK_INIT_TASK) {
                task->terminal = &null_terminal;
            } else {
                task->terminal = running_task()->terminal;  // inherit terminal from caller
            }
        }

        // Setup paging, run program loader, get new EIP. Require terminal ID.
        // NOTICE: after setting up paging for new program, arg command become useless
        if ((task->page_id = task_paging_allocate_and_set(command, &start_eip)) < 0) {
            if (task->flags & TASK_TERMINAL_OWNER) {
                gui_destroy_window(&task->terminal->win);
                terminal_vidmem_close(task->terminal->terminal_id);
                terminal_deallocate(task->terminal);
            }
            task_deallocate(task);
            return -1;
        }
        // Clean up #5 starts: task_reset_paging(running_task()->page_id, task->page_id);

        if (task->terminal->terminal_id != NULL_TERMINAL_ID) {
            terminal_fg_task[task->terminal->terminal_id] = task;  // become the user task of the terminal
            // Always set this new task as focus, even it inherits terminal from its parent that is at background
            task_set_focus_task(task);
        }
        terminal_set_running(task->terminal);  // always, because task is about to run

        if (new_terminal) {
            // Clean new terminal
            clear();
            reset_cursor();

            // Print message on new terminal
            printf("<Terminal %d>\n", task->terminal->terminal_id);  // apply to new terminal
        }

        // Clean up #6 starts: set back focus_task and running terminal
    }

    // Init RTC control info
    rtc_control_init(&task->rtc);

    // Init opened file list
    init_file_array(&task->file_array);

    // Init signals
    task_signal_init(&task->signals);

    /** --------------- Phase 3. Ready to go. Setup scheduler --------------- */

    // Put child task into run queue
    if (task->flags & TASK_IDLE_TASK) {
        task->sched_ctrl.remain_time = 0;
    } else {
        sched_refill_time(task);
    }


    sched_insert_to_head_unsafe(task);
    // Don't use launch() function of sched, perform context switch manually as follows

    // Put current task into list for parents that wait for child to return
    if (wait_for_return == 1) {
        // Safe to use running_task() since init task should not wait for return, which has been checked above
        running_task()->flags |= TASK_WAITING_CHILD;
        // Already in lock
        sched_move_running_after_node_unsafe(&wait4child_list);

    }

    /** --------------- Phase 4. Very ready to go. Do assembly level switch --------------- */

    // Set tss to new task's kernel stack to make sure system calls use correct stack
    // Whenever switch from user to kernel stack, kernel stack should be clean, so tss.esp0 should always be kesp_base
    tss.esp0 = task->kesp_base;

    // Jump to user program entry
    if (task->flags & TASK_KERNEL_TASK) {
        if (task->flags & TASK_INIT_TASK) {
            execute_launch_in_kernel(temp, task->kesp, start_eip, program_ret);
        } else {
            execute_launch_in_kernel(running_task()->kesp, task->kesp, start_eip, program_ret);
        }
    } else {
        if (task->flags & TASK_INIT_TASK) {
            execute_launch(temp, USER_STACK_STARTING_ADDR, start_eip, program_ret);
        } else {
            execute_launch(running_task()->kesp, USER_STACK_STARTING_ADDR, start_eip, program_ret);
        }
    }

    // The child task running...

    /** --------------- Phase 5. This task get reactivated --------------- */

    // Sanity check. TASK_WAITING_CHILD should be cleared when this thread is re-activated
    // Safe to use running_task() since init task should never reach here. It should get stuck at halt()
    if (running_task()->flags & TASK_WAITING_CHILD) {
        DEBUG_ERR("task of %s is still waiting for exit of child and should not be waken up!",
                  running_task()->executable_name);
    }

    // Running task is already set by whatever active this task

#if TASK_ENABLE_CHECKPOINT
    checkpoint_task_paging_consistent();  // check paging is recovered to current task
#endif

    return program_ret;
}

/**
 * Actual implementation of halt() system call
 * @param status    Exit code of current task (size are enlarged to support 256 return from exception)
 * @note Wrap this function with a lock (although it will not return)
 * @return This function should never return
 */
int32_t system_halt(int32_t status) {

    // This whole function is wrapped in a lock

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

    // Print an empty line at halt
    putc('\n');

    /** --------------- Phase 1. Remove current task from scheduler --------------- */


    sched_move_running_after_node_unsafe(&temp_list);

    if (parent) {
        // Re-activate parent
        parent->flags &= ~TASK_WAITING_CHILD;
        sched_refill_time(parent);
        // Already in lock
        sched_insert_to_head_unsafe(parent);
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
    int term_id = task->terminal->terminal_id;
    if (task->flags & TASK_TERMINAL_OWNER) {
        terminal_fg_task[term_id] = NULL;
        if (focus_task_->terminal->terminal_id == term_id) {
            int new_term_id = get_another_term_id(term_id);
            if (new_term_id == NULL_TERMINAL_ID) {
                task_set_focus_task(NULL);
            } else if (new_term_id != -1) {
                task_set_focus_task(terminal_fg_task[new_term_id]);
            } else {
                task_set_focus_task(NULL);
                DEBUG_ERR("system_halt(): error when finding another another available terminal");
            }
        }
        gui_destroy_window(&task->terminal->win);  // destroy window
        terminal_vidmem_close(term_id);  // deallocate terminal video memory
        terminal_deallocate(task->terminal);  // deallocate terminal control block
    } else {
        if (parent->terminal->terminal_id != term_id) {
            DEBUG_ERR("system_halt(): task uses terminal %d but not own it, but its parent uses terminal %d",
                      term_id, parent->terminal->terminal_id);
        }
        terminal_fg_task[parent->terminal->terminal_id] = parent;
        if (focus_task_->terminal->terminal_id == parent->terminal->terminal_id) {
            task_set_focus_task(parent);
        }
    }

    // Deallocate task
    task_deallocate(task);

    /** --------------- Phase 3. Ready to go. Do low level switch --------------- */

    if (parent) {

        task_paging_deallocate(task->page_id);
        if (parent->page_id != -1) {
            task_paging_set(parent->page_id);  // switch page to parent
        }

        // Always set this new task as focus, even it inherits terminal from its parent that is at background
        terminal_set_running(parent->terminal);

        tss.esp0 = parent->kesp_base;  // set tss to parent's kernel stack to make sure system calls use correct stack

        // It's OK to leave the lock there. After returning to parent, parent's flags will be recover
        halt_backtrack(parent->kesp, status);

        // Goodbye world...

    } else {  // sadly, its parent don't want this thread to return to it, so we yield to whoever want to run

        sched_launch_to_current_head();

        // Goodbye world...
    }

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

/**
 * Main function of init task
 * @usage Kernel task EIP of init task
 * @note When this function is executed, IF flag should be 1, otherwise idle task won't be able to yield processor
 *       to other task
 */
static void init_task_main() {

    uint32_t flags;
    cli_and_save(flags);
    {
        // It's OK to lock the whole function. New program will have flags with IF = 1.
        system_execute((uint8_t *) "idle", -1, 0, idle_task_main);

        system_execute((uint8_t *) "shell", 0, 1, NULL);
//        system_execute((uint8_t *) "shell", 0, 1, NULL);
//        system_execute((uint8_t *) "shell", 0, 1, NULL);

    }
    restore_flags(flags);

    while (1) {
        if (task_count == 2) {
            cli_and_save(flags);
            {
                system_execute((uint8_t *) "shell", 0, 1, NULL);
                terminal_focus_printf("<Last shell has halted. Restarted.>\n");  // put in newly started terminal
            }
            restore_flags(flags);
        } else {
            cli_and_save(flags);
            {
                sched_yield_unsafe();
            }
            restore_flags(flags);
        }
    }

    cli_and_save(flags);
    {
        system_halt(-1);
        // If the halt doesn't return, it won't cause a problem
    }
    restore_flags(flags);
}

/**
 * Main function of idle task
 * @usage Kernel task EIP of init task
 * @note When this function is executed, IF flag should be 1, otherwise idle task won't be able to yield processor
 *       to other task
 */
static void idle_task_main() {
    while (1) {}

    uint32_t flags;
    cli_and_save(flags);
    {
        system_halt(-1);
        // If the halt doesn't return, it won't cause a problem
    }
    restore_flags(flags);
}

task_t *focus_task() {
#if TASK_ENABLE_CHECKPOINT
    if (focus_task_ == NULL) {
        DEBUG_ERR("focus_task is NULL!");
    }
#endif
    return focus_task_;
}

/**
 * Interface for terminal driver to switch terminal
 * @param terminal_id    ID of terminal to switch to
 */
void task_change_focus(int32_t terminal_id) {
    if (terminal_id < 0 || terminal_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("task_change_focus(): invalid terminal_id");
        return;
    }

    if (terminal_fg_task[terminal_id] == NULL) {
        terminal_focus_printf("<No terminal %d>\n", terminal_id);
        return;
    }

    task_set_focus_task(terminal_fg_task[terminal_id]);

//    terminal_focus_printf("<Now in terminal %d>\n", terminal_id);
}

/**
 * Change focus task and switch video memory
 * @param task    New focus task, can be NULL
 */
static void task_set_focus_task(task_t *task) {

    if (task != NULL && task->terminal == NULL) {
        DEBUG_ERR("task_set_focus_task(): task is not NULL but its terminal is NULL, which should never happen");
        return;
    }

    uint32_t flags;

    cli_and_save(flags);
    {
        // Don't need to switch physical memory anymore with SVGA

        focus_task_ = task;

        // User vidmap won't change since they all maps to buffer
    }
    restore_flags(flags);
}

void task_apply_user_vidmap(task_t *task) {
    if (task == NULL) {
        DEBUG_ERR("task_apply_user_vidmap(): NULL task");
        return;
    }
    if (task->vidmap_enabled) {
        if (task->terminal->terminal_id == NULL_TERMINAL_ID) {
            DEBUG_ERR("task_apply_user_vidmap(): task has no terminal but vidmap is enabled");
            return;
        }
        task_set_user_vidmap(task->terminal->terminal_id);
    } else {
        task_set_user_vidmap(NULL_TERMINAL_ID);
    }
}

void move_task_to_list_unsafe(task_t *task, task_list_node_t *new_prev, task_list_node_t *new_next) {
    task_list_node_t *n = &task->list_node;
    n->next->prev = n->prev;
    n->prev->next = n->next;
    n->prev = new_prev;
    new_prev->next = n;
    n->next = new_next;
    new_next->prev = n;
}

void move_task_after_node_unsafe(task_t *task, task_list_node_t *node) {
    move_task_to_list_unsafe(task, node, node->next);
}

/**
 * Get another term ID that has foreground task
 * @param term_id_start
 * @return
 */
static int get_another_term_id(int term_id_start) {
    if (term_id_start < 0 || term_id_start >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("get_another_term_id(): invalid term_id_start");
        return -1;
    }

    int i;
    for (i = 1; i < TERMINAL_MAX_COUNT; i++) {
        if (terminal_fg_task[(term_id_start - i + TERMINAL_MAX_COUNT) % TERMINAL_MAX_COUNT] != NULL) {
            return (term_id_start - i + TERMINAL_MAX_COUNT) % TERMINAL_MAX_COUNT;
        }
    }

    return NULL_TERMINAL_ID;
}
