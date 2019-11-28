//
// Created by liuzikai on 11/12/19.
//

#ifndef _TASK_SCHED_H
#define _TASK_SCHED_H

#ifndef ASM

#include "types.h"
#include "task.h"

#define _SCHED_ENABLE_RUN_QUEUE_CHECK    1

#define SCHED_PIT_FREQUENCY    100  // Hz
#define SCHED_PIT_INTERVAL     (1000 / SCHED_PIT_FREQUENCY)  // ms
#define SCHED_TASK_TIME        50  // ms

void sched_init();
void sched_refill_time(task_t* task);
void sched_insert_to_head(task_t* task);
void sched_move_running_to_list(task_list_node_t* new_prev, task_list_node_t* new_next);
void sched_move_running_after_node(task_list_node_t* node);
void sched_launch_to_current_head();
void sched_request_run_head_asap();

#endif // ASM
#endif // _TASK_SCHED_H
