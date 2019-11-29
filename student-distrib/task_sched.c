//
// Created by liuzikai on 11/12/19.
//

#include "task_sched.h"

#include "lib.h"
#include "idt.h"
#include "task.h"
#include "task_paging.h"

task_list_node_t run_queue = TASK_LIST_SENTINEL(run_queue);

#if _SCHED_ENABLE_RUN_QUEUE_CHECK

void _sched_check_run_queue() {
    if (task_count == 0) {
        DEBUG_ERR("No running task!");
    }
    if (running_task()->list_node.prev != &run_queue || run_queue.next != &running_task()->list_node) {
        DEBUG_ERR("Sched run queue is inconsistent!");
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
 * @note This function works even called when IF = 0, since all flags will be saved and restored. But make sure
 *       there is no other mechanism that prevent interrupts from happening, such as EOI of i8259.
 */
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
 * Refill remain time of a task
 * @param task   The task to be refilled
 */
void sched_refill_time(task_t *task) {
    task->sched_ctrl.remain_time = SCHED_TASK_TIME;
}

/**
 * Insert a task to the head of running queue
 * @param task    The task to be insert
 * @note Not includes refilling the time of the task
 * @note Not includes performing low-level context switch
 */
void sched_insert_to_head(task_t *task) {
    move_task_after_node(task, &run_queue);  // move the task from whatever list to run queue head
}

/**
 * Perform low-level context switch to current head. Return after caller to this function is active again.
 */
void sched_launch_to_current_head() {

    uint32_t flags;
    task_t *to_run;

    if (run_queue.next == &run_queue) {  // nothing to run
        DEBUG_ERR("sched_launch_to_current_head(): run queue should never be empty!");
        return;
    }

    cli_and_save(flags);
    {
        to_run = task_from_node(run_queue.next);  // localize this variable, in case that run queue changes
        if (to_run->flags & TASK_IDLE_TASK) {  // the head is idle task
            if (to_run->list_node.next != &run_queue) {  // there are still other task to run
                move_task_after_node(to_run, run_queue.prev);  // move idle task to last
                to_run = task_from_node(to_run->list_node.next);
            }  // if no other task is runnable, run idle task
        }
    }
    restore_flags(flags);

    // If they are the same, do nothing
    if (running_task() == to_run) return;

    if (to_run->terminal) {
        terminal_vid_set(to_run->terminal->terminal_id);
    }

    sched_launch_to(running_task()->kesp, to_run->kesp);
    // Another task running... Until this task get running again!
}

/**
 * Move running task to the end of the run queue
 * @note Always use running_task() instead of first element in run queue, to allow lock-free
 */
void sched_move_running_to_last() {
    uint32_t flags;

    /*
     * Since we need an if before move_task_after_node, the lock in move_task_after_node can't take effect immediately,
     * so we need a lock to protect them from outside.
     */
    cli_and_save(flags);
    {
        /*
         * Be very careful since it moves task in the same list. Without this if, when run_queue has only current running
         * task, run_queue.prev will be running_task itself, and it will be completely detached from run queue.
         */
        if (run_queue.prev != &running_task()->list_node) {
            move_task_after_node(running_task(), run_queue.prev);
        }
    }
    restore_flags(flags);

}

/**
 * Interrupt handler for PIT
 * @usage Used in idt_asm.S
 * @note Always use running_task() instead of first element in run queue, to allow lock-free
 */
asmlinkage void sched_pit_interrupt_handler(uint32_t irq_num) {

    if (run_queue.next == &run_queue) {  // no runnable task
        idt_send_eoi(irq_num);
        return;
    }

    task_t *running = running_task();

    if (running->flags & TASK_IDLE_TASK) {

        sched_move_running_to_last();

        idt_send_eoi(irq_num); // must send EOI before context switch, or PIT won't work in new task
        sched_launch_to_current_head();  // return after this thread get running again

    } else {

        // Decrease available time of current running task
        running->sched_ctrl.remain_time -= SCHED_PIT_INTERVAL;

        if (running->sched_ctrl.remain_time <= 0) {  // running_task runs out of its time
            /*
             *  Re-fill remain time when putting a task to the end, instead of when getting it to running.
             *  For example, task A have 30 ms left, but task B was inserted to the head of run queue because of rtc read()
             *  complete, etc. After B runs out of its time, A should have 30ms, rather than re-filling it with 50 ms.
             */
            sched_refill_time(running);
            sched_move_running_to_last();

            idt_send_eoi(irq_num); // must send EOI before context switch, or PIT won't work in new task
            sched_launch_to_current_head();  // return after this thread get running again
        } else {
            idt_send_eoi(irq_num);
        }
    }
}

/**
 * Start PIT interrupt
 * @param hz    PIT clock frequency
 * @note Reference: http://www.osdever.net/bkerndev/Docs/pit.htm
 */
static void setup_pit(uint16_t hz) {
    uint16_t divisor = 1193180 / hz;
    outb(0x36, 0x34);  // set command byte 0x36
    outb(divisor & 0xFF, 0x40);   // Set low byte of divisor
    outb(divisor >> 8, 0x40);     // Set high byte of divisor
}

static int sched_print_run_queue() {
    int count = 0;
    task_list_node_t* node;
    task_list_for_each(node, &run_queue) {
        printf("[%d] %s\n", count++, task_from_node(node)->executable_name);
    }
    return count;
}
