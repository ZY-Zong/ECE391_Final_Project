/*
 * File system functions
 * handle the operatoins to pre-loaded file system module
 */

#ifndef     _FILE_SYSTEM_H
#define     _FILE_SYSTEM_H

#include "multiboot.h"
#include "types.h"

/*
 * Version 2.0 Tingkai Liu 2019.10.26
 * first written 
 * 
 * Version 3.0 Tingkai Liu 2019.10.27
 * support system call 
 */

#define     MAX_OPEN_FILE   8

#define     FD_IN_USE       42
#define     FD_NOT_IN_USE   0


#define     FILE_BLOCK_SIZE_IN_BYTES    4096
#define     DIR_ENTRY_SIZE_IN_BYTES     64
#define     DATA_BLOCK_NUM_SIZE_IN_BYTE 4

#define     FILE_NAME_LENGTH    32

/************************* file system (abtraction) structs *********************/

// function pointers for system calls 
typedef int32_t(*func_open)(const uint8_t*);
typedef int32_t(*func_close)(int32_t);
typedef int32_t(*func_read)(int32_t, void*, int32_t);
typedef int32_t(*func_write)(int32_t, const void*, int32_t);

// struct for operation table
typedef struct operation_table_t {
    func_open    open;
    func_close   close;
    func_read    read;
    func_write   write;
} operation_table_t;

// the struct for process control block, see mp3 document 8.2
typedef struct pcb_t {
    operation_table_t*    file_op_table_p;    // 4 Bytes: file operatoin table pointer
    uint32_t    inode;              // 4 Bytes: the inode number, only valid for data file
    uint32_t    file_position;      // 4 Bytes: where is currently reading, updated by sys read
    uint32_t    flags;              // 4 Bytes: making this file descriptor as "in use"
} pcb_t;


/*************************** file system (utility) structs *****************************/

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
    uint32_t    length_in_bytes;    // 4 Bytes: the length of the file in Byte
    uint32_t    data_block_num[(FILE_BLOCK_SIZE_IN_BYTES / DATA_BLOCK_NUM_SIZE_IN_BYTE) - 1];
                                // the numbers of data blocks that belong to this file
                                // only the first length_in_bytes Bytes are valid
} inode_t ;

// the struct for data block, see mp3 document 8.1 
typedef struct data_block_t {   // the total size of a data block: 4kB 
    uint8_t     data[FILE_BLOCK_SIZE_IN_BYTES];
} data_block_t ;


/*************************** abstract file system calls ***********************/

int32_t file_system_open(const uint8_t* filename);
int32_t file_system_close(int32_t fd);
int32_t file_system_read(int32_t fd, void* buf, int32_t nBytes);
int32_t file_system_write(int32_t fd, const void* buf, int32_t nBytes);


/***************************** public functions *********************************/


int32_t init_file_system(module_t * file_system);

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


/**************************** file operatoins ****************************/

int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void* buf, int32_t nBytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nBytes);


/**************************** directory operatoins ****************************/

int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void* buf, int32_t nBytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nBytes);


/******************************* extra support ***************************/

int32_t local_rtc_open(const uint8_t* filename);
int32_t local_rtc_close(int32_t fd);
int32_t local_rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t local_rtc_write(int32_t fd, const void* buf, int32_t nbytes);

// for test 
int32_t get_file_size(uint32_t inode);


#endif  // _FILE_SYSTEM_H
