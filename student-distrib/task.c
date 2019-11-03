//
// Created by liuzikai on 11/2/19.
//

#include "task.h"
#include "lib.h"

uint32_t process_cnt = 0;
process_t *cur_process = NULL;

/**
 * This macro yield CPU to new process and return after it terminate
 * @note Make sure paging of the new process is all set
 * @note Make sure TSS is set to kernel stack of this new process
 */
#define execute_launch(kesp_save_to, new_esp, new_eip, ret) asm (" \
    pushl $1f       /* return address to label 1, on top of the stack after iret */ \n\
    movl %%esp, %0\
    pushl $USER_DS  /* user SS  */ \n\
    pushl %2        /* user ESP */ \n\
    pushf           /* flags (new program should not care) */ \n\
    pushl $USER_CS  /* user CS  */ \n\
    pushl %3        /* user EIP */ \n\
    iret            /* enter user program */ \n\
1:  movl %%eax, %1  /* return value pass by halt() in EAX */ "  \
    : "=rm" (kesp_save_to), "=rm" (ret) \
    : "rm" (new_esp), "rm" (new_eip) \
    : "cc", "memory" \
    )


/**
 * This jump back to label 1 in execute_launch
 * @note Make sure paging of the destination process is all set
 * @note Make sure TSS is set to kernel stack of the destination process
 */
#define halt_backtrack(old_esp) asm (" \
    movl %0, %%esp  /* load back old ESP */ \n\
    ret  /* on the old kernel stack, return address is on the top of the stack */" \
    : \
    : "r" (old_esp) \
    : "cc", "memory" \
    )

/**
 * Initialize process list
 */
void process_init() {
    int i;
    for (i = 0; i < PROCESS_MAX_CNT; i++) {
        ptr_process(i)->valid = 0;
    }
}

/**
 * Find an available slot in process list, mark as valid and return. If no available, return NULL
 * @return Pointer to newly allocated process_t, or NULL is no available
 */
static process_t *process_allocate_new_slot() {
    int i;

    if (process_cnt >= PROCESS_MAX_CNT) return NULL;

    process_cnt++;
    for (i = 0; i < PROCESS_MAX_CNT; i++) {
        if (!ptr_process(i)->valid) {
            ptr_process(i)->valid = 1;
            return ptr_process(i);
        }
    }
    printf(PRINT_ERR "process_cnt is inconsistent");
    return NULL;
}

/**
 * Create a new process in the process list and init process control block
 * @return Pointer to process_t
 */
process_t *process_create() {
    process_t *proc = process_allocate_new_slot();  // allocate a new PCB

    if (proc == NULL) return NULL;  // no available slot

    proc->opened_files =;  // init opened file list
    proc->parent = cur_process;
    proc->kesp = ((uint32_t) proc) + PKM_SIZE_IN_BYTES;  // initialize kernel esp to the bottom of PKM

    return proc;
}

/**
 * Remove a process from process list
 * @param proc    Pointer to process_t of the process to be removed
 * @return Pointer to its parent process
 */
process_t *process_remove_from_list(process_t *proc) {
    process_t *ret = proc->parent;
    proc->valid = 0;
    return ret;
}

/**
 * Helper function to parse command into executable name and argument string
 * @param command    [In] string to be parse. [Out] executable name
 * @param args       [Out] pointer to string of arguments
 * @return 0 if success
 */
static int32_t execute_parse_command(uint8_t *command, uint8_t **args) {
    while (*command != '\0') {
        if (*command == ' ') {
            *command = '\0';  // split executable name and args;
            *args = command + 1;  // output args string
            return 0;
        }
        command++;  // move to next character
    }
    // No args
    *args = NULL;
    return 0;
}

/**
 * Actual implementation of execute() system call
 * @param command    Command to be executed
 * @return Terminate status of the program (0-255 if program terminate by calling halt(), 256 if exception occurs)
 * @note New program given in command will run immediately, and this function will return after its terminate
 */
int32_t system_execute(uint8_t *command) {

    process_t *proc = process_create();
    uint32_t start_eip;
    int32_t program_ret;

    if (proc == NULL) return -1;  // failed to create new process

    if (execute_parse_command(command, &proc->args) != 0) return -1;  // invalid command

    // TODO: allocate new page

    if ((start_eip = /* TODO: open executable file */) == 0) {  // invalid executable file
        // TODO: deallocate the page
        return -1;
    }

    tss.esp0 = proc->kesp;  // set tss to new process's kernel stack to make sure system calls use correct stack
    cur_process = proc;  // switch to the new process

    execute_launch(proc->parent->kesp, USER_STACK_STARTING_ADDR, start_eip, program_ret);

    return program_ret;
}

/**
 * Actual implementation of halt() system call
 * @param status    Exit code of current process
 * @return This function should never return
 */
int32_t system_halt(uint8_t status) {

    // TODO: closed all opened files

    process_t *parent = process_remove_from_list(cur_process);
    if (parent == NULL) {  // the last program has been halt
        printf("OS halt with status %u", status);
        while (1) {}
    }

    // TODO: switch page to parent

    tss.esp0 = parent->kesp;  // set tss to parent's kernel stack to make sure system calls use correct stack
    cur_process = parent;   // switch to the parent process

    halt_backtrack(parent->kesp);

    printf(PRINT_ERR "halt() should never return");

    return -1;
}

/**
 * Actual implementation of getargs() system call
 * @param buf       String buffer to accept args
 * @param nbytes    Maximal number of bytes to write to buf
 * @return 0 on success, -1 on no argument or argument string can't fit in nbytes
 */
int32_t system_getargs(uint8_t *buf, int32_t nbytes) {
    if (cur_process->args == NULL) return -1;  // no args
    if (strlen((int8_t *) cur_process->args) >= nbytes) return -1;  // can not fit into buf (including ending NULL char)
    strncpy((int8_t *) buf, (int8_t *) cur_process->args, nbytes);
    return 0;
}