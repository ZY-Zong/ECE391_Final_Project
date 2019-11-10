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
    file_array_t file_array;
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

#define PKM_ALIGN_MASK     0xFFFF2000  // PKM is 8k-aligned, when in kernel_stack, mask ESP with this is current PCB

// Address of idx-th process control block
#define ptr_process(idx) ((process_t *) (PKM_STARTING_ADDR - (idx + 1) * PKM_SIZE_IN_BYTES))
#define PROCESS_MAX_CNT    2  // maximum number of processes running at the same time
extern uint32_t process_cnt;

extern inline process_t* cur_process();  // get current process based on ESP, only used in kernel state

#define USER_STACK_STARTING_ADDR  (0x8400000 - 1)  // User stack starts at 132MB - 1 (with paging enabled)

void process_init();
process_t* process_create();
process_t* process_remove_from_list(process_t* proc);

int32_t system_execute(uint8_t *command);
int32_t system_halt(uint8_t status);
int32_t system_getargs(uint8_t *buf, int32_t nbytes);

#endif // _TASK_H
