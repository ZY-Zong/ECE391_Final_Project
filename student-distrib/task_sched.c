//
// Created by liuzikai on 11/12/19.
//

#include "task_sched.h"

#include "lib.h"
#include "task.h"

task_list_node_t run_queue = TASK_LIST_SENTINEL(run_queue);

uint32_t switch_asap_count = 0;

// TODO: confirm sync of run_queue carefully

#if _SCHED_ENABLE_RUN_QUEUE_CHECK
void _sched_check_run_queue() {
    if (task_count == 0) {
        DEBUG_ERR("No running task!");
    }
    if (switch_asap_count == 0) {  // if an interrupt request to switch to a task asap, it will be insert to the head
        if (running_task()->list_node.prev != &run_queue || run_queue.next != &running_task()->list_node) {
            DEBUG_ERR("Sched run queue is inconsistent!");
        }
    }
}
#else
#define _sched_check_run_queue()    do {} while(0)
#endif

/**
 * This macro yield CPU from current process (_prev_) to new process (_next_) and prepare kernel stack for return
 * @param kesp_save_to    Save ESP of kernel stack of _prev_ to this address
 * @param new_kesp        Starting ESP of kernel stack of _next_
 * @note Make sure paging of _next_ is all set
 * @note Make sure TSS is set to kernel stack of _next_
 * @note After switching, the top of _prev_ stack is the return address (label 1)
 * @note To switch back, switch stack to kernel stack of _prev_, and run `ret` on _prev_ stack
 */
// TODO: confirm IF is persistent after switch
#define sched_launch_to(kesp_save_to, new_kesp) asm volatile ("                                             \
    pushfl          /* save flags on the stack */                                                         \n\
    pushl %%ebp     /* save EBP on the stack */                                                           \n\
    pushl $1f       /* return address to label 1, on top of the stack after iret */                       \n\
    movl %%esp, %0  /* save current ESP */                                                                \n\
    movl %1, %%esp  /* swap kernel stack */                                                               \n\
    ret             /* using the return address on the top of new kernel stack */                         \n\
1:  popl %%ebp      /* restore EBP, must before following instructions */                                 \n\
    popfl           /* restore flags */"                                                                    \
    : "=m" (kesp_save_to) /* must write to memory, since this macro returns after task get running again */ \
    : "m" (new_kesp)      /* must read from memory, in case two process are the same */                     \
    : "cc", "memory"                                                                                        \
)

static void setup_pit(uint16_t hz);

/**
 * Initialize scheduler
 */
void sched_init() {
    setup_pit(SCHED_PIT_FREQUENCY);
}

/**
 * Move current running task to an external list, mostly a wait list
 * @param new_prev    Pointer to new prev node
 * @param new_next    Pointer to new next node
 * @note Not includes performing low-level context switch
 */
void sched_move_running_to_list(task_list_node_t* new_prev, task_list_node_t* new_next) {
    _sched_check_run_queue();
    move_task_to_list(running_task(), new_prev, new_next);
}

/**
 * Move current running task after a node, mostly in a wait list
 * @param node    Pointer to the new prev node
 * @note Not includes performing low-level context switch
 */
void sched_move_running_after_node(task_list_node_t* node) {
    _sched_check_run_queue();
    sched_move_running_to_list(node, node->next);
}


/**
 * Refill remain time of a task
 * @param task   The task to be refilled
 */
void sched_refill_time(task_t* task) {
    task->sched_ctrl.remain_time = SCHED_TASK_TIME;
}

/**
 * Insert a task to the head of running queue
 * @param task    The task to be insert
 * @note Not includes refilling the time of the task
 * @note Not includes performing low-level context switch
 */
void sched_insert_to_head(task_t* task) {
    _sched_check_run_queue();
    move_task_after_node(task, &run_queue);  // move the task from whatever list to run queue head
}


void sched_launch_to_current_head() {
    // If they are the same, do nothing
    if (running_task() == task_from_node(run_queue.next)) return;

    sched_launch_to(running_task()->kesp, task_from_node(run_queue.next)->kesp);
    // Another task running... Until this task get running again!
}

void sched_request_run_head_asap() {
    uint32_t flags;
    cli_and_save(flags); {
        switch_asap_count++;
    } restore_flags(flags);
}

void sched_move_running_to_last() {
    _sched_check_run_queue();
    /*
     * Be very careful since it moves task in the same list. Without this if, when run_queue has only current running
     * task, run_queue.prev will be running_task itself, and it will be completely detached from run queue.
     */
    if (run_queue.prev != &running_task()->list_node) {
        move_task_after_node(running_task(), run_queue.prev);
    }

}

/**
 * Interrupt handler for PIT
 * @usage Used in idt_asm.S
 */
void sched_pit_interrupt_handler() {
#if _SCHED_ENABLE_RUN_QUEUE_CHECK

#endif
    task_t* running = running_task();

    // Decrease available time of current running task
    running->sched_ctrl.remain_time -= SCHED_PIT_INTERVAL;

    while (switch_asap_count > 0) {



        switch_asap_count--;
    }

    if (running->sched_ctrl.remain_time <= 0) {  // running_task runs out of its time

        _sched_check_run_queue();

        /*
         *  Re-fill remain time when putting a task to the end, instead of when getting it to running.
         *  For example, task A have 30 ms left, but task B was inserted to the head of run queue because of rtc read()
         *  complete, etc. After B runs out of its time, A should have 30ms, rather than re-filling it with 50 ms.
         */
        sched_refill_time(running);
        sched_move_running_to_last();

        sched_launch_to_current_head();  // return after this thread get running again
    }
}

/**
 * Start PIT interrupt
 * @param hz    PIT clock frequency
 * @note Reference: http://www.osdever.net/bkerndev/Docs/pit.htm

 */
static void setup_pit(uint16_t hz)
{
    uint16_t divisor = 1193180 / hz;
    outb(0x36, 0x34);  // set command byte 0x36
    outb(divisor & 0xFF, 0x40);   // Set low byte of divisor
    outb(divisor >> 8, 0x40);     // Set high byte of divisor
}
