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
void sched_insert_to_head_unsafe(task_t* task);
void sched_move_running_to_list_unsafe(task_list_node_t* new_prev, task_list_node_t* new_next);
void sched_move_running_after_node_unsafe(task_list_node_t* node);

void sched_launch_to_current_head();

#endif // ASM
#endif // _TASK_SCHED_H
