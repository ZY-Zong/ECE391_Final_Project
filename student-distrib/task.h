//
// Created by liuzikai on 11/2/19.
//

#ifndef _TASK_H
#define _TASK_H

#include "file_system.h"

// Task control block (TCB) stores info for every task
typedef struct {
    file_array_t opened_files;
} task_control_block_t;


// Task kernel memory (TKM) consists of task control block and the kernel stack for a task

#define TKM_SIZE_IN_BYTES    8192

typedef struct {
    task_control_block_t tcb;
    uint8_t kstack[TKM_SIZE_IN_BYTES - sizeof(task_control_block_t)];  // stack for kernel status
} task_kernel_memory_t;

#endif //_TASK_H
