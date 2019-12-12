/*
 * Deal with the paging turn on and load for task 
 * Note: 
 *  kernel page is done by hard code in x86_desc.S
 *  init_paging is done in boot.S 
 */

#ifndef _VIDMEM_H
#define _TERMINAL_VIDMEM_H

#include "../types.h"

/*
 * Version 3.0 Tingkai Liu 2019.11.3
 * First written 
 * 
 * Version 4.0 Tingkai Liu 2019.11.17
 * Support system call: vidmap 
 *
 * Version 5.0 Tingkai Liu 2019.11.27
 * Support multi-terminal and scheduling
 * 
 * Version 6.0 Tingkai Liu 2019.12.4
 * Support sVGA by moving the place for the tasks downwards 
 */

void task_paging_init();
int task_paging_allocate_and_set(const uint8_t *task_name, uint32_t *eip);  // called by system call execute
int task_paging_deallocate(const int page_id);  // called by system call halt
int task_paging_set(const int page_id); // call by running task for its page

#endif /*_VIDMEM_H*/

