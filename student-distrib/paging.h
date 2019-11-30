//
// Created by liuzikai on 11/29/19.
//

#ifndef _PAGING_H
#define _PAGING_H

#include "x86_desc.h"

int clear_PDE_4MB(PDE_4MB_t *entry);
int clear_PDE_4kB(PDE_4kB_t *entry);
int clear_PTE(PTE_t *entry);
int set_PDE_4kB(PDE_4kB_t *pde, uint32_t pt, uint8_t can_write, uint8_t user, uint8_t present);
int set_PTE(PTE_t *pte, uint32_t page, uint8_t can_write, uint8_t user, uint8_t present);

#endif //_PAGING_H
