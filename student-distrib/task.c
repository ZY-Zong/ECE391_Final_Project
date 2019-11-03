//
// Created by liuzikai on 11/2/19.
//

#include "task.h"
#include "lib.h"

uint32_t process_cnt = 0;
process_t* cur_process = NULL;

static const uint8_t* empty_args = "";

process_t* process_create() {

    if (process_cnt >= PROCESS_MAX_CNT) return NULL;

    process_t* proc = ptr_process(process_cnt++);  // allocate a new PCB

    proc->opened_files = ;  // init opened file list
    proc->parent = cur_process;
    proc->kesp = ((uint32_t) proc) + PKM_SIZE_IN_BYTES;  // initialize kernel esp to the bottom of PKM

    return proc;
}

void process_remove() {

}

int32_t execute_parse_command(uint8_t* command, uint8_t** args) {
    while (*command != '\0') {
        if (*command == ' ') {
            *command = '\0';  // split executable name and args;
            *args = command + 1;  // output args string
            return 0;
        }
        command++;  // move to next character
    }
    // No args
    *args = empty_args;
    return 0;
}

int32_t system_execute(uint8_t *command) {

    process_t* proc = process_create();
    uint32_t start_eip;
    int32_t program_ret;

    if (proc == NULL) return -1;  // failed to create new process

    if (execute_parse_command(command, &proc->args) != 0) return -1;  // invalid command

    if ((start_eip = /* TODO: open executable file */) == 0) return -1;  // invalid executable file

    execute_launch(USER_STACK_STARTING_ADDR, start_eip, program_ret);

    return program_ret;
}

int32_t system_halt(uint8_t status) {



    printf(PRINT_ERR "halt() should never return");

    return -1;
}

