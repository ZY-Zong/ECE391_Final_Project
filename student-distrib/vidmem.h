//
// Created by liuzikai on 11/29/19.
//

#ifndef _VIDMEM_H
#define _VIDMEM_H

#include "types.h"
#include "paging.h"

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

void vidmem_init();
int terminal_vidmem_open(const int term_id);
int terminal_vidmem_close(const int term_id);
int terminal_vidmem_switch_active(const int new_active_id, const int pre_active_id);
int terminal_vidmem_set(const int term_id);

int system_vidmap(uint8_t ** screen_start);

void task_set_user_vidmap(int term_id);

#endif //_VIDMEM_H
