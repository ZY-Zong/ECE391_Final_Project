
/*
 * Deal with the paging turn on and load for task 
 * Note: 
 *  kernal page is done by hard code in x86_desc.S
 *  init_paging is done in boot.S 
 */

#ifndef _TASK_PAHING_H
#define _TASK_PAHING_H

#include "types.h"

/*
 * Version 3.0 Tingkai Liu 2019.11.3
 * First written 
 */

#define     MAX_RUNNING_TASK    2           // for cp3 
#define     PID_USED            666         // good number!
#define     PID_FREE            0   

#define     TASK_START_MEM      0x08000000  // 128MB
#define     TASK_IMG_LOAD_ADDR  0x08048000
#define     TASK_PAGE_FLAG      0x00000087  // flags for a user level task 
#define     TASK_VIR_MEM_ENTRY  32          // 128MB / 4MB


int task_set_up_paging(const uint8_t* task_name, uint32_t* eip); // called by system call execute
int task_reset_paging(const int cur_id, const int pre_id); // called by system call halt 



#endif /*_TASK_PAHING_H*/

