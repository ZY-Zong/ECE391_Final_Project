
#include "file_system.h"
#include "lib.h"


// global variables for file system 
static module_t file_system;    // the module for file system 
static int  file_system_inited=0;   // flag for file system inited or not 
                                    // to avoid double init 
                                    // 0 for not init, 1 for inited 

// blocks for file system 
static boot_block_t boot_block;
static inode_t* inodes = NULL ;
static data_block_t* data_blocks = NULL;


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
 * do nothing in cp2
 */
int32_t file_open(const uint8_t* filename){
    (void)filename;
    return 0;
}

/*
 * file_close()
 * do nothing in cp2
 */
int32_t file_close(int32_t fd){
    (void)fd;
    return 0;
}

int32_t file_read(){
    return 0;
}

int32_t file_write(){
    return 0;
}



/**************************** directory operatoins ****************************/

int32_t dir_open(){
    return 0;
}

int32_t dir_close(){
    return 0;
}

int32_t dir_read(){
    return 0;
}

int32_t dir_write(){
    return 0;
}

