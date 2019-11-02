
#ifndef _TASK_PAHING_H
#define _TASK_PAHING_H

/*
 * Deal with the paging turn on and load for task 
 * Note: 
 *  kernal page is done by hard code in x86_desc.S
 *  init_paging is done in boot.S 
 */

int task_turn_on_paging(); // called by system call execute 
int task_turn_off_paging(); // called by system call halt 



/***************************** helper funtion *******************************/

int task_is_executable();
int task_load();


#endif /*_TASK_PAHING_H*/

