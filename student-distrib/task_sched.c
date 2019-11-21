//
// Created by liuzikai on 11/12/19.
//

#include "task_sched.h"

#include "lib.h"
#include "task.h"

task_list_node_t run_queue = TASK_LIST_SENTINEL(run_queue);

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
#define sched_switch_to(kesp_save_to, new_kesp) asm volatile ("                       \
    pushfl          /* save flags on the stack */                                   \n\
    pushl %%ebp     /* save EBP on the stack */                                     \n\
    pushl $1f       /* return address to label 1, on top of the stack after iret */ \n\
    movl %%esp, %0  /* save current ESP */                                          \n\
    movl %1, %%esp  /* swap kernel stack */                                         \n\
    ret             /* using the return address on the top of new kernel stack */   \n\
1:  popl %%ebp      /* restore EBP, must before following instructions */           \n\
    popfl           /* restore flags */"                                              \
    : "=m" (kesp_save_to) /* must write to memory, or halt() will not get it */       \
    : "rm" (new_kesp)                                                                 \
    : "cc", "memory"                                                                  \
)

static void setup_pit(int hz);

void sched_init() {

}

void sched_refill_time(task_t* task) {
    task->sched_ctrl.remain_time = SCHED_TASK_TIME;
}

void sched_insert_to_head(task_t* task) {
    move_task_after_node(task, &run_queue);  // move the task from whatever list to run queue
}

void sched_switch_to_current_head() {

}

void sched_pit_callback() {

}

/**
 * Start PIT interrupt
 * @param hz    PIT clock frequency
 * @note Reference: http://www.osdever.net/bkerndev/Docs/pit.htm

 */
static void setup_pit(int hz)
{
    uint32_t divisor = 1193180 / hz;
    outb(0x43, 0x36);  // set command byte 0x36
    outb(0x40, divisor & 0xFF);   // Set low byte of divisor
    outb(0x40, divisor >> 8);     // Set high byte of divisor
}
