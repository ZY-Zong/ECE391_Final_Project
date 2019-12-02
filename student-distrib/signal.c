
#include "signal.h"
#include "task.h"
#include "lib.h"

extern void signal_set_up_stack_helper(int32_t signum, hw_context_t *hw_context_addr, signal_handler handler);

#define     IS_SIGNAL(signal)   ( (signal >= 0) && (signal < MAX_NUM_SIGNAL) )

// Global Variable
signal_handler default_handlers[MAX_NUM_SIGNAL];

int32_t signal_div_zero_default_handler();

int32_t signal_segfault_default_handler();

int32_t signal_interrupt_default_handler();

int32_t signal_alarm_default_handler();

int32_t signal_user1_default_handler();

int32_t signal_behavior_kill();

/*************************** System Calls ***************************/

/**
 * Set the handler for specific signal 
 * @param       signum: the signal number to be set 
 * @param       handler_address: the address of the handler, which is a function pointer  
 *                              NULL if want to set the default handler  
 * @return      0 for success, -1 for fail 
 * @effect      since the whole OS share a same handler array, will affect the behavior of all
 */
int32_t system_set_handler(int32_t signum, void *handler_address) {
    if (!IS_SIGNAL(signum)) {
        DEBUG_ERR("system_set_handler(): bad signum: %d\n", signum);
        return -1;
    }

    uint32_t flags;
    cli_and_save(flags);
    {
        if (handler_address == NULL) {
            running_task()->signals.current_handlers[signum] = default_handlers[signum];
        } else {
            running_task()->signals.current_handlers[signum] = (signal_handler) handler_address;
        }
    }
    restore_flags(flags);


    return 0;
}


/*************************** Signal Operations ***************************/

/**
 * Init all the signal handlers
 * Should be called at boot 
 */
void signal_init() {
    default_handlers[SIGNAL_DIV_ZERO] = signal_div_zero_default_handler;
    default_handlers[SIGNAL_SEGFAULT] = signal_segfault_default_handler;
    default_handlers[SIGNAL_INTERRUPT] = signal_interrupt_default_handler;
    default_handlers[SIGNAL_ALARM] = signal_alarm_default_handler;
    default_handlers[SIGNAL_USER1] = signal_user1_default_handler;
}


/**
 * Init a signal struct by setting pending and mask to 0 and current handler to default 
 * @param       signal_struct: the signal struct to be init 
 * @return      0 for success, -1 for bad signal_struct
 */
int32_t task_signal_init(signal_struct_t *signal_struct) {
    if (signal_struct == NULL) {
        DEBUG_ERR("signal_init(): bad signal_struct!\n");
        return -1;
    }

    signal_struct->pending_signal = 0;
    signal_struct->masked_signal = 0;

    int i; // loop counter 
    for (i = 0; i < MAX_NUM_SIGNAL; i++) {
        signal_struct->current_handlers[i] = default_handlers[i];
    }


    return 0;
}

/**
 * Send a signal to current running task by updating the pending field 
 * Should only call by kernel 
 * @return      0 for success, -1 for fail
 * @note        will not check whether this signal already sent, as desired
 */
int32_t signal_send(int32_t signal) {
    if (!IS_SIGNAL(signal)) {
        DEBUG_ERR("signal_send(): invalid signal number: %d\n", signal);
        return -1;
    }

    uint32_t flags;
    cli_and_save(flags);
    {
        running_task()->signals.pending_signal |= 1 << signal;
    }

    return 0;
}

/**
 * Block a signal for current running task by updating the block field 
 * Should only call by kernel 
 * @return      0 for success, -1 for fail
 */
int32_t signal_block(int32_t signal) {
    if (!IS_SIGNAL(signal)) {
        DEBUG_ERR("signal_block(): invalid signal number: %d\n", signal);
        return -1;
    }

    uint32_t flags;
    cli_and_save(flags);
    {
        running_task()->signals.masked_signal |= 1 << signal;
    }
    restore_flags(flags);

    return 0;
}

/**
 * Unblock a signal for current running task by updating the block field 
 * Should only call by kernel 
 * @return      0 for success, -1 for fail
 */
int32_t signal_unblock(int32_t signal) {
    if (!IS_SIGNAL(signal)) {
        DEBUG_ERR("signal_unblock(): invalid signal number: %d\n", signal);
        return -1;
    }

    uint32_t flags;
    cli_and_save(flags);
    {
        running_task()->signals.masked_signal &= ~(1 << signal);
    }
    restore_flags(flags);

    return 0;
}

/**
 * Check whether there is a signal pending 
 * If yes, set up the stack frame for signal handler 
 * @note        Should be called every time return from exception, interrupt and system call
 * @note        In our simplify version of signal, only one signal can be sent at a time
 */
asmlinkage void signal_check(hw_context_t context) {

    int32_t flag;
    cli_and_save(flag);
    {
        uint32_t cur_signal = running_task()->signals.pending_signal & (~running_task()->signals.masked_signal);

        // Check whether there is a signal pending that is not blocked
        if (cur_signal == 0) return;

        // Get the signal number
        uint32_t cur_signal_num;
        for (cur_signal_num = 0; cur_signal_num < MAX_NUM_SIGNAL; cur_signal_num++) {
            if (cur_signal & 0x1) break;
            cur_signal = cur_signal >> 1;
        }

        // Mask all signals and store previous mask
        running_task()->signals.available = running_task()->signals.masked_signal;
        running_task()->signals.masked_signal = SIGNAL_MASK_ALL;
        running_task()->signals.pending_signal = 0; // clear the signal

        // Set up the stack frame if needed 
        if (running_task()->signals.current_handlers[cur_signal_num] == default_handlers[cur_signal_num]) {
            default_handlers[cur_signal_num]();
        } else {
            signal_set_up_stack_helper(cur_signal_num, &context,
                                       running_task()->signals.current_handlers[cur_signal_num]);
        }

    }
    restore_flags(flag);


}

/*************************** Signal Handlers  ***************************/

/**
 * Default handler for SIGNAL_DIV_ZERO
 * Default action: kill 
 */
int32_t signal_div_zero_default_handler() {
    return signal_behavior_kill();
}

/**
 * Default handler for SIGNAL_SEGFAULT
 * Default action: kill 
 */
int32_t signal_segfault_default_handler() {
    return signal_behavior_kill();
}

/**
 * Default handler for SIGNAL_INTERRUPT
 * Default action: kill 
 */
int32_t signal_interrupt_default_handler() {
    return signal_behavior_kill();
}

/**
 * Default handler for SIGNAL_ALARM
 * Default action: ignore
 */
int32_t signal_alarm_default_handler() {
    return 0;
}

/**
 * Default handler for SIGNAL_USER1
 * Default action: ignore 
 */
int32_t signal_user1_default_handler() {
    return 0;
}

/*************************** Helper Functions  ***************************/

/**
 * Help function that halt current process 
 * @return     this function should never return  
 */
int32_t signal_behavior_kill() {
    system_halt(256);
    // Halt should not return
    return -1;
}

/**
 * Restore previous mask 
 * called by system call sigreturn 
 */
void signal_restore_mask(void) {
    uint32_t flags;
    cli_and_save(flags);
    {
        running_task()->signals.masked_signal = running_task()->signals.available;
    }
    restore_flags(flags);
}
