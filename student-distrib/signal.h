
#ifndef     _SIGNAL_H
#define     _SIGNAL_H


/*
 * Version 1.0 Tingkai Liu 2019.11.30 
 * First written 
 */

/*
 * Situations that the signal should be send by calling signal_send()
 * SIGNAL_DIV_ZERO:
 *      processor generated divide_by_zero exception while executing user level code 
 * SIGNAL_SEGFAULT: 
 *      any other exception occurred
 * SIGNAL_INTERRUPT:
 *      ctrl+c is pressed
 * SIGNAL_ALARM:  
 *      send to current executing task (focus task) every 10 seconds 
 * SIGNAL_USER1:
 *      when user want 
 */

#define     MAX_NUM_SIGNAL      5 

#define     SIGNAL_DIV_ZERO     0
#define     SIGNAL_SEGFAULT     1 
#define     SIGNAL_INTERRUPT    2 
#define     SIGNAL_ALARM        3 
#define     SIGNAL_USER1        4 

#define     SIGNAL_MASK_ALL     0xFFFFFFFF

#ifndef     ASM_SIGNAL

#include "types.h"
#include "idt.h"

typedef int32_t (*signal_handler)(void);

typedef struct signal_struct_t {
    uint32_t    pending_signal;     // bit k is 1 means signal k is pending
    uint32_t    masked_signal;      // bit k is 1 means signal k is masked
    uint32_t    available;          // could be used to restore mask after handling
    signal_handler current_handlers[MAX_NUM_SIGNAL];
} signal_struct_t;

// System calls
int32_t system_set_handler(int32_t signum, void* handler_address);
extern int32_t system_sigreturn(void);  // defined in signal_asm.S

// Signal operations
void signal_init(); // call at boot

int32_t task_signal_init(signal_struct_t* signal_struct);  // init signal fields when execute a task
int32_t signal_send(int32_t signal);  // send a signal to running task
int32_t signal_block(int32_t signal);  // block a signal for running task
int32_t signal_unblock(int32_t signal);  // unblock a signal for running task

// Signal handlers
int32_t signal_handle(int32_t signal);
 
#endif /* ASM */
#endif /* ifndef _SIGNAL_H */
