
#include "file_system.h"
#include "lib.h"


// global variables for file system 
static module_t file_system;    // the module for file system 
static int  file_system_inited=0;   // flag for file system inited or not 
                                    // to avoid double init 
                                    // 0 for not init, 1 for inited 
// operation tables 
static operation_table_t RTC_op_table;
static operation_table_t dir_op_table;
static operation_table_t file_op_table;

// PCB array 
static int32_t current_open_file_num = 2; // 0 and 1 for stdin and stdout  
static PCB_t    opened_files[MAX_OPEN_FILE];


// blocks for file system 
static boot_block_t boot_block;
static inode_t* inodes = NULL ;
static data_block_t* data_blocks = NULL;


/*************************** abtract file system calls ***********************/

/*
 * file_system_open()
 * support system call: open 
 * call funtion corresponding to file type 
 * see system call for explanation
 */
int32_t file_system_open(const uint8_t* filename){
    // check if already open max number of file 
    if (current_open_file_num >= MAX_OPEN_FILE) {
        printf("WARNING: file_open(): already reach max, cannot open %s\n", filename);
        return -1;
    }
    
    // find the correspond dentry 
    dentry_t    current_dentry;
    if (-1 == read_dentry_by_name(filename, &current_dentry) ){
        printf("WARNING: file_open(): cannot open %s, no such file\n", filename);
    }

    int32_t fd = -1; // the fd to return

    // call open function according to file type 
    switch (current_dentry.file_type){
        case 0: // RTC
            fd = RTC_open(filename); 
            break;
        case 1: // directory
            fd = dir_open(filename);
            break;
        case 2: // regular file 
            fd = file_open(filename);
            break;
        default:
            printf("ERROR: file_open(): unknown file type of %s\n", filename);
            file_close(fd) ;
            return -1; 
    }

    current_open_file_num++;

    return fd;
}

/*
 * file_system_close()
 * support system call: close 
 * close the file and release the fd 
 * INPUTS: 
 * the file to be closed 
 * RETURN:
 * 0 for success
 * -1 for fail 
 * SIDE EFFECTS:
 * the array of PCB and count will be changed 
 */
int32_t file_system_close(int32_t fd){
    // check whether fd is valid 
    if (fd == 0){
        printf("ERROR: file_system_close(): cannot close stdin\n");
        return -1;
    }
    if (fd == 1){
        printf("ERROR: file_system_close(): cannot close stdout\n");
        return -1;
    }
    if ( fd < 0 || fd > MAX_OPEN_FILE){
        printf("ERROR: file_system_close(): no such fd!\n");
        return -1;
    }

    opened_files[fd].flags=FD_NOT_IN_USE;
    return 0;
}

/*
 * file_system_read()
 * support system call: read
 * call funtion corresponding to file type 
 * have different thing to read for different type of files 
 * read the things of file fd into buf 
 * INPUTS:
 * int32_t fd: the file 
 * void* buf: the output buffer 
 * int32_t nBytes: the number to Bytes to read 
 * RETURN:
 * the number of Bytes read and placed into buffer 
 * -1 for the bad fd
 * (note: 0 if offset reach the end of the file)
 * SIDE EFFECTS:
 * buf will be changed 
 * the file position filed of corresponding PCB will be changed 
 * TODO: keyboard support 
 */
int32_t file_system_read(int32_t fd, void* buf, int32_t nBytes){
    // check whether the file is opened 
    if (opened_files[fd].flags == FD_NOT_IN_USE){
        printf("ERROR: file_system_read(): fd %d is not opened\n", fd);
        return -1;
    }

    return opened_files[fd].file_op_table_p->read(fd, buf, nBytes);
}

/*
 * file_system_write()
 * support system call: write 
 * call funtion corresponding to file type 
 * different file type have different thing to write 
 * note: this is a read only file system, if file type is dir/regular file, return -1
 * INPUTS:
 * int32_t fd: the file 
 * const void* buf: the things to write 
 * int32_t nBytes: the number of Bytes to write 
 * RETURN:
 * 0 for success 
 * -1 for fail 
 * TODO: terminal support 
 */
int32_t file_system_write(int32_t fd, const void* buf, int32_t nBytes){
    // check whether the file is opened 
    if (opened_files[fd].flags == FD_NOT_IN_USE){
        printf("ERROR: file_system_write(): fd %d is not opened\n", fd);
        return -1;
    }

    return opened_files[fd].file_op_table_p->write(fd, buf, nBytes);
}


/***************************** public functions *********************************/

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
int32_t init_file_system(module_t * fs){
    // check if already inited 
    if (file_system_inited == 1){
        printf("ERROR: file_system.c: file system already inited\n");
        return -1;
    }

    // init: set the global variables 
    file_system_inited = 1 ;
    file_system= *fs ;

    // store the blocks as global variable 
    boot_block = * ( (boot_block_t *)fs->mod_start ) ;
    inodes = (inode_t *)fs->mod_start +1 ;
    data_blocks = (data_block_t *)fs->mod_start + boot_block.inode_num + 1 ;

    // init PCB 
    int i; // loop counter 
    for (i=0; i< MAX_OPEN_FILE; i++){
        opened_files[i].flags=FD_NOT_IN_USE;
    }
    // TODO: init stdin and stdout ?

    // init operation table for RTC 
    RTC_op_table.open=RTC_open;
    RTC_op_table.close=RTC_close;
    RTC_op_table.read=RTC_read;
    RTC_op_table.write=RTC_write;

    // init operation table for dir 
    dir_op_table.open=dir_open;
    dir_op_table.close=dir_close;
    dir_op_table.read=dir_read;
    dir_op_table.write=dir_write;

    // init operation table for regular file 
    file_op_table.open=file_open;
    file_op_table.close=file_close;
    file_op_table.read=file_read;
    file_op_table.write=file_write;


    return 0;
}

/*
 * read_dentry_by_name()
 * find the directory entry in the file system with name: fname 
 * and store it into *dentry
 * INPUTS:
 * const uint8_t* fname: the file name of the entry 
 * dentry_t* dentry: pointer of output dentry 
 * RETURNS:
 * 0 for success 
 * -1 for no such file exist 
 * SIDE EFFECTS:
 * the *dentry will be changed 
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    
    int i; // loop counter 

    // loop though all the file names 
    for (i=0; i < boot_block.dir_num; i++){ 
        // test whether current file match 
        if ( ! strncmp( (int8_t*)fname, (int8_t*)boot_block.dir_entries[i].file_name, FILE_NAME_LENGTH)){
            // if yes, set dentry and return 
            *dentry = boot_block.dir_entries[i];
            return 0;
        }
    }
    
    // after looping though all dentries and not found, meaning not exist 
    return -1;
}

/*
 * read_dentry_by_index()
 * find the directory entry in the file system with index: index 
 * and store it into *dentry
 * NOTE: in the file system boot block, the index start from 0
 * INPUTS:
 * uint32_t index: the index of the dentry 
 * dentry_t* dentry: pointer of output dentry 
 * RETURNS:
 * 0 for success 
 * -1 for no such index exist 
 * SIDE EFFECTS:
 * the *dentry will be changed
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    
    int i; // loop counter

    // loop though all the file names  
    for (i=0; i<boot_block.dir_num; i++){
        // test whether current file match
        if (index == boot_block.dir_entries[i].inode_num){
            // if yes, set dentry and return
            *dentry = boot_block.dir_entries[i];
            return 0;
        }
    }
    
    // after looping though all dentries and not found, meaning not exist 
    return -1;
}

/*
 * read_data()
 * read length Bytes from the poition offset in the needed file 
 * the file has inode number: inode 
 * place the data read into the buffer 
 * IMPROTANT: the caller is responsible for produce enough space for the buffer 
 * INPUTS:
 * uint32_t inode: the inode number of the file to be read 
 * uint32_t offset: the position begin to be read in the file 
 * uint8_t* buf: the output buffer 
 * uint32_t length: the length to read 
 * RETURN:
 * the number of Bytes read and placed into buffer 
 * -1 for the bad inode / inode point to bad data block
 * (note: 0 if offset reach the end of the file)
 * SIDE EFFECTS:
 * the buf will be changed 
 * the offset will be changed 
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    
    // check whether inode is valid 
    if (inode >= boot_block.inode_num) return -1;
   
    uint32_t file_length = inodes[inode].length_in_B; // the max length of the file

    // counters, all of them, including offset needed to be updated in the loop 
    uint32_t Bytes_read = 0 ; // a counter for how many Bytes have been read 
    // data block 
    uint32_t current_data_block_index = (offset / FILE_BLOCK_SIZE_IN_BYTES) - 1; 
    uint32_t current_data_block_num = inodes[inode].data_block_num[current_data_block_index];                           
    uint32_t current_data_block_offset = offset - (current_data_block_index * FILE_BLOCK_SIZE_IN_BYTES);

    // check if the data block is valid 
    if (current_data_block_num >= boot_block.data_block_num) return -1;

    while (Bytes_read < length){
        // check if reach the end of the file 
        if (offset >= file_length) break ; 

        // copy the current Byte 
        buf[Bytes_read] = data_blocks[current_data_block_num].data[current_data_block_offset];

        // update the counters 
        Bytes_read++ ;
        offset++ ; 
        current_data_block_offset++ ; 
        // check if need to update block_num 
        if (current_data_block_offset >= FILE_BLOCK_SIZE_IN_BYTES){
            
            current_data_block_offset -= FILE_BLOCK_SIZE_IN_BYTES ;
            current_data_block_index++ ;
            current_data_block_num = inodes[inode].data_block_num[current_data_block_index] ; 

            // check if the data block is valid 
            if (current_data_block_num >= boot_block.data_block_num) return -1;
        }

    }
    
    return Bytes_read;
}



/**************************** file operatoins ****************************/

/*
 * file_open()
 * get a PCB for the file and return its file descriptor number 
 * garentee by file_system_open that have space
 * INPUTS:
 * const uint8_t* filename: the name of the file to open 
 * RETURN:
 * the file descriptor (fd) of the file 
 */
int32_t file_open(const uint8_t* filename){
    // find the correspond dentry 
    dentry_t    current_dentry;
    if (-1 == read_dentry_by_name(filename, &current_dentry) ){
        printf("WARNING: file_open(): cannot open %s, no such file\n", filename);
    }
    
    // get a PCB
    int32_t fd = -1; // the fd to return
    for (fd=2; fd < MAX_OPEN_FILE; fd++){
        if (opened_files[fd].flags == FD_NOT_IN_USE){
            opened_files[fd].flags = FD_IN_USE ;
            break;
        }
    }

    // init the PCB got 
    opened_files[fd].file_op_table_p = &file_op_table;
    opened_files[fd].inode = current_dentry.inode_num;
    opened_files[fd].file_position = 0; // the beginning of the file 
    opened_files[fd].flags = FD_IN_USE;

    return fd;
}

/*
 * file_close()
 * should not be called becasue close is already handled by system call 
 * have this for consistancy
 * always report error and return -1
 */
int32_t file_close(int32_t fd){
    printf("ERROR: file_close() should not be called\n");
    printf("please use system call to close file with fd: %d", fd);
    return -1;
}

/*
 * file_read()
 * read to the end of the file or the end of the buffer 
 * whichever occurs sonner 
 * INPUTS:
 * int32_t fd: the file to read
 * void* buf: the output buffer 
 * int32_t nBytes: the size of the buffer 
 * RETURN:
 * the number of Bytes read and placed into buffer 
 * -1 for the bad inode / inode point to bad data block
 * (note: 0 if offset reach the end of the file)
 * SIDE EFFECTS:
 * the buf will be changed 
 * the file position filed of corresponding PCB will be changed  
 */
int32_t file_read(int32_t fd, void* buf, int32_t nBytes){
    int32_t ret=0;  // the return value 
    uint32_t offset= opened_files[fd].file_position; // current offset of the file 

    // place the data into buffer 
    ret=read_data(opened_files[fd].inode, offset, buf, nBytes); 
    // check if success 
    if (ret == -1) return -1;

    // update the file position
    opened_files[fd].file_position += ret;

    return ret;
}

/*
 * file_write()
 * the file system is read only, always return -1 and report error 
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nBytes){
    // paras will not be used, avoid warning
    (void)fd; (void)buf; (void)nBytes;

    printf("ERROR: file_write(): the file system is read only\n");
    return -1;
}



/**************************** directory operatoins ****************************/

/*
 * dir_open()
 * get a PCB for the dir and return its file descriptor number 
 * garentee by file_system_open that have space
 * INPUTS:
 * const uint8_t* filename: the name of the dir to open 
 * RETURN:
 * the file descriptor (fd) of the dir
 */
int32_t dir_open(const uint8_t* filename){
    (void) filename; // no need to use, avoid warning 

    // get a PCB
    int32_t fd = -1; // the fd to return
    for (fd=2; fd < MAX_OPEN_FILE; fd++){
        if (opened_files[fd].flags == FD_NOT_IN_USE){
            opened_files[fd].flags = FD_IN_USE ;
            break;
        }
    }

    // init the PCB got 
    opened_files[fd].file_op_table_p = &dir_op_table;
    opened_files[fd].inode = 0;
    opened_files[fd].file_position = 0; // the beginning of the file 
    opened_files[fd].flags = FD_IN_USE;

    return fd;
}

/*
 * dir_close()
 * should not be called becasue close is already handled by system call 
 * have this for consistancy
 * always report error and return -1
 */
int32_t dir_close(int32_t fd){
    printf("ERROR: dir_close() should not be called\n");
    printf("please use system call to close dir with fd: %d", fd);
    return -1;
}

/*
 * dir_read()
 * read the dentries' name in current dir from index: PCB.file_position
 * report if reach the end 
 * INPUTS: 
 * int32_t fd: the directory
 * void* buf: the output buffer 
 * int32_t nBytes: meaningless 
 * RETURN:
 * FILE_NAME_LENGTH if success 
 * 0 if reach the end 
 * SIDE EFFECTS:
 * the buf will be changed 
 * the file position filed of corresponding PCB will be changed  
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nBytes){   
    // since only one dir exist, just get from boot block 
    // check if reach the end 
    if (opened_files[fd].file_position>=boot_block.dir_num){
        printf("WARNING: dir_read(): already reach the end\n");
        return 0;
    }

    // copy the file name into buf 
    strncpy(buf, (int8_t*)(boot_block.dir_entries[opened_files[fd].file_position++].file_name), FILE_NAME_LENGTH);
    
    return FILE_NAME_LENGTH;
}

/*
 * dir_write()
 * the file system is read only, always return -1 and report error 
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nBytes){
    // paras will not be used, avoid warning
    (void)fd; (void)buf; (void)nBytes;

    printf("ERROR: dir_write(): the file system is read only\n");
    return -1;
}

/******************************* extra support ***************************/

/*
 * RTC_open()
 * get a PCB for the RTC and return its file descriptor number 
 * garentee by file_system_open that have space
 * INPUTS:
 * const uint8_t* filename: the name of the RTC to open 
 * RETURN:
 * the file descriptor (fd) of the RTC 
 */
int32_t RTC_open(const uint8_t* filename){
    (void) filename; // no need to use, avoid warning 

    // get a PCB
    int32_t fd = -1; // the fd to return
    for (fd=2; fd < MAX_OPEN_FILE; fd++){
        if (opened_files[fd].flags == FD_NOT_IN_USE){
            opened_files[fd].flags = FD_IN_USE ;
            break;
        }
    }

    // init the PCB got 
    opened_files[fd].file_op_table_p = &RTC_op_table;
    opened_files[fd].inode = 0;
    opened_files[fd].file_position = 0; // the beginning of the file 
    opened_files[fd].flags = FD_IN_USE;
    
    return fd;
}

/*
 * RTC_close()
 * should not be called becasue close is already handled by system call 
 * have this for consistancy
 * always report error and return -1
 */
int32_t RTC_close(int32_t fd){
    printf("ERROR: RTC_close() should not be called\n");
    printf("please use system call to close RTC with fd: %d", fd);
    return -1;
}

