
#ifndef     _SIGNAL_H
#define     _SIGNAL_H

#include "types.h"

/*
 * Version 
 */

#define     MAX_NUM_SIGNAL      5 

#define     SIGNAL_DIV_ZERO     0
#define     SIGNAL_SEGFAULT     1 
#define     SIGNAL_INTERRUPT    2 
#define     SIGNAL_ALARM        3 
#define     SIGNAL_USER1        4 

#define     SIGNAL_MASK_ALL     0xFFFFFFFF

typedef struct signal_struct_t {
    int32_t     pending_signal;     // bit k is 1 means signal k is pending 
    int32_t     masked_signal;      // bit k is 1 means signal k is masked 
    int32_t     available;          // could be used to restore mask after handlering
} signal_struct_t;

// System calls
int32_t system_set_handler(int32_t signum, void* handler_address);
int32_t system_sigreturn(void);

// Signal operations 
int32_t signal_init(signal_struct_t* signal_struct); // init signal fileds when execute a task
int32_t signal_send(int32_t signal); // send a signal to running task
int32_t signal_block(int32_t signal); // block a signal for running task
int32_t signal_unblock(int32_t signal); // unblock a signal for running task
void signal_check(); // check whether there is signal pending and do work if yes, call before iret

// Signal handlers
int32_t signal_handle(int32_t signal);
int32_t signal_div_zero_default_handler();
int32_t signal_segfault_default_handler();
int32_t signal_interrupt_default_handler();
int32_t signal_alarm_default_handler();

#endif /* ifndef _SIGNAL_H */
