//
// Created by liuzikai on 11/2/19.
//

#ifndef _TASK_H
#define _TASK_H

#include "types.h"
#include "x86_desc.h"
#include "file_system.h"

// Process control block (PCB) stores info for every task
typedef struct process_t process_t;
struct process_t {
    uint8_t valid;
    process_t* parent;
    uint8_t* args;
    uint32_t kesp;  // kernel stack esp
    file_array_t opened_files;
    int page_id;
};


// Process kernel memory (PKM) consists of task control block and the kernel stack for a task
#define PKM_SIZE_IN_BYTES    8192
typedef union process_kernel_memory_t process_kernel_memory_t;
union process_kernel_memory_t {
    process_t tcb;
    uint8_t kstack[PKM_SIZE_IN_BYTES];  // stack for kernel status
};

#define PKM_STARTING_ADDR    0x800000  // PKM starts at 8MB (bottom of kernel image), going to low address


#define ptr_process(idx) (((process_t *) PKM_STARTING_ADDR) - (idx + 1))  // address of idx-th process control block
#define PROCESS_MAX_CNT    2  // maximum number of processes running at the same time
extern uint32_t process_cnt;

extern process_t* cur_process;  // current process

#define USER_STACK_STARTING_ADDR  0x8400000  // User stack starts at 132MB (with paging)

void process_init();
process_t* process_create();
process_t* process_remove_from_list(process_t* proc);

int32_t system_execute(uint8_t *command);
int32_t system_halt(uint8_t status);
int32_t system_getargs(uint8_t *buf, int32_t nbytes);

#endif // _TASK_H