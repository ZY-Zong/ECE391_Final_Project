//
// Created by liuzikai on 11/2/19.
//

#include "task.h"
#include "lib.h"
#include "file_system.h"
#include "task_paging.h"

#define USER_STACK_STARTING_ADDR  (0x8400000 - 1)  // User stack starts at 132MB - 1 (with paging enabled)

uint32_t process_cnt = 0;

/**
 * This macro yield CPU from current process (_prev_) to new process (_next_) and return after _next_ terminate
 * @param kesp_save_to    Save ESP of kernel stack of _prev_ to this address
 * @param new_esp         Starting ESP of _next_
 * @param new_eip         Starting EIP of _next_
 * @param ret             After _next_ terminate, return value is written to this address, and this macro returns
 * @note Make sure paging of _next_ is all set
 * @note Make sure TSS is set to kernel stack of _next_
 * @note After switching, the top of _prev_ stack is the return address (label 1)
 * @note To switch back, load the return value in EAX, switch stack to _prev_, and run `ret` on _prev_ stack
 */
#define execute_launch(kesp_save_to, new_esp, new_eip, ret) asm volatile ("                    \
    pushfl          /* save flags on the stack */                                   \n\
    pushl %%ebp     /* save EBP on the stack */                                     \n\
    pushl $1f       /* return address to label 1, on top of the stack after iret */ \n\
    movl %%esp, %0  /* save current ESP */                                          \n\
    /* The following stack linkage is for IRET */                                   \n\
    pushl $0x002B   /* user SS - USER_DS */                                         \n\
    pushl %2        /* user ESP */                                                  \n\
    pushf           /* flags (new program should not care) */                       \n\
    pushl $0x0023   /* user CS - USER_CS  */                                        \n\
    pushl %3        /* user EIP */                                                  \n\
    iret            /* enter user program */                                        \n\
1:  popl %%ebp      /* restore EBP, must before following instructions */           \n\
    movl %%eax, %1  /* return value pass by halt() in EAX */                        \n\
    popfl           /* restore flags */"                                              \
    : "=m" (kesp_save_to), /* must write to memory, or halt() will not get it */      \
      "=m" (ret)                                                                      \
    : "rm" (new_esp), "rm" (new_eip)                                                  \
    : "cc", "memory"                                                                  \
)


/**
 * This jump back to label 1 in execute_launch
 * @param old_esp    ESP of parent process. On the top of parent's stack should be return address to parent's code
 * @param ret        Return value of current thread. Will be stored to EAX
 * @note Make sure paging of the destination process is all set
 * @note Make sure TSS is set to kernel stack of the destination process
 */
#define halt_backtrack(old_esp, ret) asm volatile ("                                        \
    movl %0, %%esp  /* load back old ESP */                                      \n\
    /* EAX store the return status */                                            \n\
    ret  /* on the old kernel stack, return address is on the top of the stack */" \
    :                                                                              \
    : "r" (old_esp)   , "a" (ret)                                                  \
    : "cc", "memory"                                                               \
)

static process_t *process_remove_from_list(process_t *proc);

/**
 * Get current process based on ESP. Only for usage in kernel state.
 * @return Pointer to current process
 */
process_t* cur_process() {
    process_t* ret;
    asm volatile ("movl %%esp, %0  \n\
                   andl $0xFFFFE000, %0    /* PKM_ALIGN_MASK */" \
                   : "=r" (ret) \
                   : \
                   : "cc", "memory" \
                   );
    return ret;
}

/**
 * Initialize process list
 */
void task_init() {
    int i;
    for (i = 0; i < PROCESS_MAX_CNT; i++) {
        ptr_process(i)->valid = 0;
    }
}

/**
 * Run shell
 */
void task_run_initial_process() {
    system_execute((uint8_t *) "shell");
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

    DEBUG_ERR( "process_cnt is inconsistent");
    return NULL;
}

/**
 * Remove a process from process list
 * @param proc    Pointer to process_t of the process to be removed
 * @return Pointer to its parent process
 */
static process_t *process_remove_from_list(process_t *proc) {
    process_t *ret = proc->parent;
    proc->valid = 0;
    process_cnt--;
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

    process_t *proc;
    uint32_t start_eip;
    int32_t program_ret;
    uint32_t temp;

    // Allocate a new PCB
    if (NULL == (proc = process_allocate_new_slot())) return -1;  // no available slot

    // Setup flags and parent
    proc->flags = 0;
    if (process_cnt == 1) {  // this is the initial process
        proc->flags |= PROCESS_INITIAL;
        proc->parent = NULL;
    } else {
        proc->parent = cur_process();
    }

    // Initialize kernel ESP to the bottom of PKM
    proc->kesp = ((uint32_t) proc) + PKM_SIZE_IN_BYTES - 1;

    // Parse executable name and arguments
    if (execute_parse_command(command, &proc->args) != 0) {
        process_remove_from_list(proc);
        return -1;  // invalid command
    }
    proc->executable_name = command;

    // Store the executable name and argument string to the kernel stack of new program, or they will be inaccessible
    temp = strlen((int8_t *) proc->executable_name);
    proc->kesp -= temp;
    proc->executable_name = (uint8_t *) strcpy((int8_t *) proc->kesp, (int8_t *) proc->executable_name);

    if (proc->args != NULL){
        temp = strlen((int8_t *) proc->args);
        proc->kesp -= temp;
        proc->args = (uint8_t *) strcpy((int8_t *) proc->kesp, (int8_t *) proc->args);
    }

    // Setup paging, run program loader, get new EIP
    // NOTICE: after setting up paging for new program, command become useless
    if ((proc->page_id = task_set_up_memory(command, &start_eip)) < 0) {
        process_remove_from_list(proc);
        return -1;
    }

    // Init opened file list
    init_file_array(&proc->file_array);

    // Set tss to new process's kernel stack to make sure system calls use correct stack
    tss.ss0 = KERNEL_DS;
    tss.esp0 = proc->kesp;

    // Jump to user program entry
    if (proc->flags & PROCESS_INITIAL) {
        execute_launch(temp, USER_STACK_STARTING_ADDR, start_eip, program_ret);
    } else {
        execute_launch(cur_process()->kesp, USER_STACK_STARTING_ADDR, start_eip, program_ret);
    }

    return program_ret;
}

/**
 * Actual implementation of halt() system call
 * @param status    Exit code of current process (size are enlarged to support 256 return from exception)
 * @return This function should never return
 */
int32_t system_halt(int32_t status) {

    clear_file_array(&cur_process()->file_array);

    if (cur_process()->parent == NULL) {  // the last program has been halt
        int page_id = cur_process()->page_id;
        process_remove_from_list(cur_process());
        printf("Initial shell halt with status %u. Restarting...", status);
        task_reset_paging(page_id, page_id);  // do nothing to paging, but decrease count
        task_run_initial_process();  // execute shell again
        // Will not reach here
    }

    process_t *parent = process_remove_from_list(cur_process());

    task_reset_paging(cur_process()->page_id, parent->page_id);  // switch page to parent

    tss.esp0 = parent->kesp;  // set tss to parent's kernel stack to make sure system calls use correct stack

    halt_backtrack(parent->kesp, status);

    DEBUG_ERR( "halt() should never return");

    return -1;
}

/**
 * Actual implementation of getargs() system call
 * @param buf       String buffer to accept args
 * @param nbytes    Maximal number of bytes to write to buf
 * @return 0 on success, -1 on no argument or argument string can't fit in nbytes
 */
int32_t system_getargs(uint8_t *buf, int32_t nbytes) {
    uint8_t* args = cur_process()->args;
    if (args == NULL) return -1;  // no args
    if (strlen((int8_t *) args) >= nbytes) return -1;  // can not fit into buf (including ending NULL char)
    strncpy((int8_t *) buf, (int8_t *) args, nbytes);
    return 0;
}
