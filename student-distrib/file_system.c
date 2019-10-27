
#include "file_system.h"
#include "lib.h"


// global variables for file system 
static module_t file_system;    // the module for file system 
static int  file_system_inited=0;   // flag for file system inited or not 
                                    // to avoid double init 
                                    // 0 for not init, 1 for inited 

/*
 * init_file_system()
 * initialize the whole file system, usually called by kernel when init 
 * will check whether the system already inited, 
 * if not, init the file system and mark it as inited 
 * if yes, do nothing and report by print 
 * INPUTS:
 * module_t * fs: the file system image that is loaded as module
 * RETURN:
 * 0 for success
 * -1 for the file system already inited 
 * SIDE EFFECTS:
 * if success, the global variables for file system will be changed 
 * if fail, will print error message 
 */
int init_file_system(module_t * fs){
    // check if already inited 
    if (file_system_inited == 1){
        printf("ERROR: file_system.c: file system already inited\n");
        return -1;
    }

    // init: set the global variables 
    file_system_inited = 1 ;
    file_system= *fs ;

    return 0;
}

