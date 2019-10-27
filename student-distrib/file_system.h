/*
 * File system functions
 * handle the operatoins to pre-loaded file system module
 */

#ifndef     FILE_SYSTEM
#define     FILE_SYSTEM

#include "multiboot.h"
#include "types.h"


/*
 * Version 2.0 Tingkai Liu 2019.10.26
 * first written 
 */

/****************************** file system structs *****************************/

#define     FILE_BLOCK_SIZE_IN_BYTES    4096
#define     DIR_ENTRY_SIZE_IN_BYTES     64 
#define     DATA_BLOCK_NUM_SIZE_IN_BYTE 4

#define     FILE_NAME_LENGTH    32


// the struct for directory entry in the file system, see mp3 document 8.1 
typedef struct dentry_t{        // the total size of the directory entry: 64B
    uint8_t     file_name[FILE_NAME_LENGTH];  // 32 Bytes: file name 
    uint32_t    file_type;      // 4 Bytes: file type 
    uint32_t    inode_num;      // 4 Bytes: inode number 
    uint8_t     reserved[24];   // 24 Bytes reserved 
} dentry_t ;

// the struct for boot block. see mp3 document 8.1
typedef struct boot_block_t {   // the total size of the block: 4kB= (63+1) * 64B
    uint32_t    dir_num;        // 4 Bytes: number of directories 
    uint32_t    inode_num;      // 4 Bytes: number of inodes (N)
    uint32_t    data_block_num; // 4 Bytes: number of data blocks (D)
    uint8_t     reserved[52];   // 52 Bytes reserved 
    dentry_t    dir_entries[(FILE_BLOCK_SIZE_IN_BYTES / DIR_ENTRY_SIZE_IN_BYTES) - 1]; 
                                // all remaining spaces are for directory entires 
                                // only the first dir_num ones are meaningful 
} boot_block_t ;

// the struct for inode, see mp3 document 8.1 
typedef struct inode_t {        // the total size of the inode: 4kB = (1023 + 1) * 4B 
    uint32_t    length_in_B;    // 4 Bytes: the length of the file in Byte 
    uint32_t    data_block_num[(FILE_BLOCK_SIZE_IN_BYTES / DATA_BLOCK_NUM_SIZE_IN_BYTE) - 1];
                                // the numbers of data blocks that belong to this file 
                                // only the first length_in_B Bytes are valid 
} inode_t ;

// the struct for data block, see mp3 document 8.1 
typedef struct data_block_t {   // the total size of a data block: 4kB 
    uint8_t     data[FILE_BLOCK_SIZE_IN_BYTES]; 
} data_block_t ; 


/***************************** public functions *********************************/


int32_t init_file_system(module_t * file_system);

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


/**************************** file operatoins ****************************/

int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);
int32_t file_read();
int32_t file_write();


/**************************** directory operatoins ****************************/

int32_t dir_open();
int32_t dir_close();
int32_t dir_read();
int32_t dir_write();

#endif
