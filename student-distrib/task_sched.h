//
// Created by liuzikai on 11/12/19.
//

#ifndef _TASK_SCHED_H
#define _TASK_SCHED_H

#ifndef ASM

#include "types.h"
#include "task.h"

/**
 * SCHEDULING
 *  
 */

#define _SCHED_ENABLE_RUN_QUEUE_CHECK    1

#define SCHED_PIT_FREQUENCY    100  // frequency of PIT [Hz]
#define SCHED_PIT_INTERVAL     (1000 / SCHED_PIT_FREQUENCY)  // time quantum of scheduler [ms]
#define SCHED_TASK_TIME        50  // full available time for each task [ms]

extern task_list_node_t run_queue;

void sched_init();
void sched_refill_time(task_t* task);
void sched_insert_to_head(task_t* task);

/**
 * Move current running task to an external list, mostly a wait list (lock free)
 * @param new_prev    Pointer to new prev node
 * @param new_next    Pointer to new next node
 * @note A lock is placed inside move_task_to_list. Since macro expands as code and no variables are stored, the lock is
 *       expected to take effect immediately.
 * @note Be VERY careful when using this function to move a task in the same list. Pointers of new_prev and new_next
 *       may still be those BEFORE extracting the task due to potential compiler optimization
 * @note Not includes performing low-level context switch
 * @note Always use running_task() instead of first element in run queue, to allow lock-free
 */
#define sched_move_running_to_list(new_prev, new_next) {                                                       \
    /* Must be place at first to allow lock to take effect immediately so no need to add another lock */       \
    move_task_to_list(running_task(), (new_prev), (new_next));                                                 \
}

/**
 * Move current running task after a node, mostly in a wait list (lock free)
 * @param node    Pointer to the new prev node
 * @note Not includes performing low-level context switch
 * @note A lock is placed inside move_task_to_list. Since macro expands as code and no variables are stored, the lock is
 *       expected to take effect immediately.
 * @note Be VERY careful when using this function to move a task in the same list. Pointers of new_prev and new_next
 *       may still be those BEFORE extracting the task due to potential compiler optimization
 */
#define sched_move_running_after_node(node) {             \
    sched_move_running_to_list((node), (node)->next);     \
}                                                         \

void sched_launch_to_current_head();

#endif // ASM
#endif // _TASK_SCHED_H
