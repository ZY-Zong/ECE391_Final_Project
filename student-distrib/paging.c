/* This file contains the function necessary for paging 
 * including 
 * 1. init the paging when the OS boot
 * 2. set new page when a program is executed 
 */

/*
 * Version 1.0
 * Tingkai Liu 2019.10.20
 */

#include "paging.h"
#include "x86_desc.h" 

static int _program_executed=0;


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
    
    
    return 0;
}




