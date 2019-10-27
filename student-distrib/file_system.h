/*
 * File system functions
 * handle the operatoins to pre-loaded file system module
 */

#ifndef     FILE_SYSTEM
#define     FILE_SYSTEM

#include "multiboot.h"


/*
 * Version 2.0 Tingkai Liu 2019.10.26
 * first written 
 */


int init_file_system(module_t * file_system);


/**************************** file operatoins ****************************/

int file_open();
int file_close();
int file_write();
int file_read();


/**************************** directory operatoins ****************************/

int dir_open();
int dir_close();
int dir_write();
int dir_read();


#endif