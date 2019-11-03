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
    process_t* parent;
    uint8_t* args;
    uint32_t kesp;  // kernel stack esp
    file_array_t opened_files;
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



#define execute_launch(esp, eip, ret) asm (" \
    pushl $1f       /* return address to label 1, on top of the stack after iret */ \n\
    pushl $USER_DS  /* user SS  */ \n\
    pushl %1        /* user ESP */ \n\
    pushf           /* flags (new program should not care) */ \n\
    pushl $USER_CS  /* user CS  */ \n\
    pushl %2        /* user EIP */ \n\
    iret            /* enter user program */ \n\
1:  movl %%eax, %0" /* return value pass by halt in EAX*/ \
    : "=rm" (ret) \
    : "rm" (esp), "rm" (eip) \
    : "memory" \
)



#endif // _TASK_H
