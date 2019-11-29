
#include "signal.h"
#include "task.h"
#include "lib.h"

#define     IS_SIGNAL(signal)   ( (signal >= 0) && (signal < MAX_NUM_SIGNAL) )


process_t* running_task(); // should be deleted 

/*************************** System Calls ***************************/

int32_t system_set_handler(int32_t signum, void* handler_address){
    return 0;
}

int32_t system_sigreturn(void){
    return 0;
}

/*************************** Signal Operations ***************************/

/**
 * Init a signal struct by setting pending and mask to 0 
 * @param       signal_struct: the signal struct to be init 
 * @return      0 for success, -1 for bad signal_struct
 */
int32_t signal_init(signal_struct_t* signal_struct){
    if (signal_struct == NULL){
        DEBUG_ERR("signal_init(): bad signal_struct!\n");
        return -1;
    }

    signal_struct->pending_signal = 0;
    signal_struct->masked_signal = 0;

    return 0;
}

/**
 * Send a signal to current running task by updating the pending field 
 * Should only call by kernel 
 * @return      0 for success, -1 for fail
 * @note        will not check whether this signal already sent, as desired
 */
int32_t signal_send(int32_t signal){
    if (!IS_SIGNAL(signal)){
        DEBUG_ERR("signal_send(): invalid signal number: %d\n", signal);
        return -1;
    }

    running_task()->signals.pending_signal |= 1 << signal;

    return 0;
}

/**
 * Block a signal for current running task by updating the block field 
 * Should only call by kernel 
 * @return      0 for success, -1 for fail
 */
int32_t signal_block(int32_t signal){
    if (!IS_SIGNAL(signal)){
        DEBUG_ERR("signal_block(): invalid signal number: %d\n", signal);
        return -1;
    }

    running_task()->signals.masked_signal |= 1 << signal;

    return 0;
}

/**
 * Unblock a signal for current running task by updating the block field 
 * Should only call by kernel 
 * @return      0 for success, -1 for fail
 */
int32_t signal_unblock(int32_t signal){
    if (!IS_SIGNAL(signal)){
        DEBUG_ERR("signal_unblock(): invalid signal number: %d\n", signal);
        return -1;
    }

    running_task()->signals.masked_signal &= ~(1 << signal);

    return 0;
}

/**
 * Check whether there is a signal pending 
 * If yes, set up the stack frame for signal handler 
 * @note        should be called every time return from exception, interrupt and system call
 */
void signal_check(){
    // Check whether there is a signal pending 
    if (running_task()->signals.pending_signal == 0) return;

    // Check whether the pending signal is blocked 

    // Set up the signal handler stack frame 
    // asm
}

/*************************** Singal Handlers  ***************************/




/*************************** Helper Functions  ***************************/


