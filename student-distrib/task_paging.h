
/*
 * Deal with the paging turn on and load for task 
 * Note: 
 *  kernel page is done by hard code in x86_desc.S
 *  init_paging is done in boot.S 
 */

#ifndef _TASK_PAHING_H
#define _TASK_PAHING_H

#include "types.h"

/*
 * Version 3.0 Tingkai Liu 2019.11.3
 * First written 
 * 
 * Version 4.0 Tingkai Liu 2019.11.17
 * Support system call: vidmap 
 *
 * Version 5.0 Tingkai Liu 2019.11.27
 * Support multi-terminal and scheduling
 */

#define NULL_TERMINAL_ID    0xECE666  // used for ter_id indicating no opened terminal

void task_paging_init();
int task_set_up_memory(const uint8_t* task_name, uint32_t* eip, const int ter_id);  // called by system call execute
int task_reset_paging(const int cur_id, const int pre_id);  // called by system call halt
int task_running_paging_set(const int id); // call by running task for its page 

int system_vidmap(uint8_t ** screen_start);

int terminal_vid_open(const int ter_id);
int terminal_vid_close(const int ter_id);
int terminal_active_vid_switch(const int new_ter_id, const int pre_ter_id);
int terminal_vid_set(const int ter_id);


#endif /*_TASK_PAHING_H*/

