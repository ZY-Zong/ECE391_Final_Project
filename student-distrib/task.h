//
// Created by liuzikai on 11/2/19.
//

#ifndef _TASK_H
#define _TASK_H

#include "types.h"
#include "x86_desc.h"
#include "rtc.h"
#include "terminal.h"
#include "file_system.h"
#include "virtual_screen.h"

/** Process Control Block (PCB) */

#define TASK_FLAG_INITIAL        1U  // initial process
#define TASK_WAITING_RTC         8U  // is in waiting list of RTC
#define TASK_WAITING_TERMINAL    16U // is in waiting list of terminal

typedef struct task_t task_t;
struct task_t {
    uint8_t valid;  // 1 if current task_t is in use, 0 if not

    // Strings of executable name and arguments, stored in kernel stacks
    uint8_t* executable_name;
    uint8_t* args;

    uint32_t flags;  // flags for current task
    task_t* parent;
    uint32_t kesp;  // kernel stack ESP

    int32_t page_id;

    rtc_control_t rtc;

    terminal_control_t terminal;

    virtual_screen_t screen;

    file_array_t file_array;
};

/** Process Kernel Memory (PKM) */

// Process kernel memory (PKM) consists of task control block and the kernel stack for a task
#define PKM_SIZE_IN_BYTES    8192
typedef union process_kernel_memory_t process_kernel_memory_t;
union process_kernel_memory_t {
    task_t pcb;
    uint8_t kstack[PKM_SIZE_IN_BYTES];  // stack for kernel status
};

#define PKM_STARTING_ADDR    0x800000  // PKM starts at 8MB (bottom of kernel image), going to low address
#define PKM_ALIGN_MASK     0xFFFFE000  // PKM is 8k-aligned, when in kernel_stack, mask ESP with this is current PCB

/** Task Managements */

// Address of idx-th process control block
#define ptr_process(idx) ((task_t *) (PKM_STARTING_ADDR - (idx + 1) * PKM_SIZE_IN_BYTES))
#define PROCESS_MAX_CNT    2  // maximum number of processes running at the same time
extern uint32_t process_cnt;

// TODO: get rid of this function
extern inline task_t* cur_process();  // get current process based on ESP, only used in kernel state

// TODO: implement these two pointer
task_t* running_task;
task_t* focus_task;

// Used for waiting list
typedef struct task_list_node_t task_list_node_t;
struct task_list_node_t {
    task_t* task;
    task_list_node_t* next;
    task_list_node_t* prev;
};

void task_rtc_read_done(task_list_node_t* task);
void task_terminal_read_done(task_list_node_t* task);

/** Interface for Pure Kernel State */

void task_init();
void task_run_initial_process();

/** System Calls Implementations */

int32_t system_execute(uint8_t *command);
int32_t system_halt(int32_t status);
int32_t system_getargs(uint8_t *buf, int32_t nbytes);

#endif // _TASK_H
