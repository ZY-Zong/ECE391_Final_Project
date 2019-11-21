//
// Created by liuzikai on 11/12/19.
//

#ifndef _TASK_SCHED_H
#define _TASK_SCHED_H

#ifndef ASM

#include "types.h"
#include "task.h"

#define _SCHED_ENABLE_RUN_QUEUE_CHECK    1
#if _SCHED_ENABLE_RUN_QUEUE_CHECK
extern task_list_node_t run_queue;
#endif

#define SCHED_PIT_FREQUENCY    1000  // Hz
#define SCHED_PIT_INTERVAL     (1000 / SCHED_PIT_FREQUENCY)  // ms
#define SCHED_TASK_TIME        10  // ms

void sched_init();
void sched_refill_time(task_t* task);
void sched_insert_to_head(task_t* task);
void sched_pit_callback();

#endif // ASM
#endif // _TASK_SCHED_H
