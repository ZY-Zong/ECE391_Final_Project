/* This file contains the function necessary for paging 
 * including 
 * 1. init the paging when the OS boot
 * 2. set new page when a program is executed 
 */

/*
 * Version 1.0  Tingkai Liu 2019.10.20
 * first written
 * 
 * Version 1.1  Tingkai Liu 2019.10.20
 * change the init into static, i.e. init when get space for the table 
 * This file is no longer useful 
 */

#include "paging.h"
#include "x86_desc.h" 

static int _program_executed=0;
int i; // loop counter 

/*
 * init paging()
 * fill the PD and PT 
 * INPUTS:
 * none 
 * RETURN 
 * 0 for success, -1 for fail 
 * SIDE EFFECTS:
 * the PD and PT will be changed 
 */
int init_paging(){
    /* originally: 
     *  all bit 0 is 0: not present 
     *  all bit 2 is 0: supervisor
     */

    // set PD 
    // 0-4MB (index 0) enrty: map to a PT, each page is 4kB
    kernel_page_directory[0] |= ((unsigned)kernel_page_table_0 & 0xFFFFF000)  ;    // high 20 bits: PT pointer 
    kernel_page_directory[0] |= 0x1; // bit 0: present 
    kernel_page_directory[0] |= (0x1 << 1); // bit 1: both R and W 

    // 4-8MB (index 1) entry: map to a 4MB page at physical memory 4-8MB
    // in this case high 10 bits of PDE represent 10 most significant bit of physical address 
    kernel_page_directory[1] |= 0x1 << 22 ; // at 1*4MB 
    kernel_page_directory[1] |= 0x1; // bit 0: present
    kernel_page_directory[1] |= (0x1 << 1); // bit 1: both R and W 
    kernel_page_directory[1] |= (0x1 << 7); // bit 7: PS 

    // others: since originally filled with 0, they are viewed as not persent 

    // set PT 
    // except the video memory, others are marked as not present 
    kernel_page_table_0[0xB8]=0xB8 << 12; // video memory is at 0xB8000
    kernel_page_table_0[0xB8] |= 0x1;    // bit 0: present
    kernel_page_table_0[0xB8] |= (0x1 << 1); // bit 1: both R and W 
    
    return 0;
}




