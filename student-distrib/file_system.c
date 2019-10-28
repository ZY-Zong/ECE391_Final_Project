
#include "file_system.h"
#include "lib.h"

#include "terminal.h"
#include "rtc.h"

// Global variables for file system
static module_t file_system;    // the module for file system 
static int file_system_inited = 0;   // o avoid double init 0 for not init, 1 for inited

// Operation tables
static operation_table_t terminal_op_table;
static operation_table_t rtc_op_table;
static operation_table_t dir_op_table;
static operation_table_t file_op_table;

// PCB array 
static int32_t current_open_file_num = 2;  // 0 and 1 for stdin and stdout
static pcb_t opened_files[MAX_OPEN_FILE];

// Blocks for file system
static boot_block_t boot_block;
static inode_t *inodes = NULL;
static data_block_t *data_blocks = NULL;


/**
 * Get free fd in opened_files[]
 * @return Free fd, or MAX_OPEN_FILE if no available
 */
int32_t get_free_fd() {
    int fd;
    for (fd = 2; fd < MAX_OPEN_FILE; fd++) {
        if (opened_files[fd].flags == FD_NOT_IN_USE) {
            opened_files[fd].flags = FD_IN_USE;
            break;
        }
    }
    return fd;
}

/*************************** Abstract File System Calls ***********************/

/**
 * Support system call: open
 * @param filename    File to open
 * @return   -1 for failure, or file descriptor
 */
int32_t file_system_open(const uint8_t *filename) {

    // Check if already open max number of file
    if (current_open_file_num >= MAX_OPEN_FILE) {
        printf("WARNING: file_open(): already reach max, cannot open %s\n", filename);
        return -1;
    }

    // Find the correspond dentry
    dentry_t current_dentry;
    if (-1 == read_dentry_by_name(filename, &current_dentry)) {
        printf("WARNING: file_open(): cannot open %s, no such file\n", filename);
        return -1;
    }

    int32_t fd = -1;  // the fd to return

    // Call open function according to file type
    switch (current_dentry.file_type) {
        case 0:  // RTC
            fd = local_rtc_open(filename);
            break;
        case 1:  // directory
            fd = dir_open(filename);
            break;
        case 2:  // regular file
            fd = file_open(filename);
            break;
        default:
            printf("ERROR: file_open(): unknown file type of %s\n", filename);
            return -1;
    }

    current_open_file_num++;

    return fd;
}

/**
 * Support system call: close
 * @param fd    the file to be closed
 * @return      0 for success, -1 for failure
 * @effect      The array of PCB and count will be changed
 */
int32_t file_system_close(int32_t fd) {

    int32_t ret;

    // Check whether fd is valid
    if (fd == 0) {
        printf("ERROR: file_system_close(): cannot close stdin\n");
        return -1;
    }
    if (fd == 1) {
        printf("ERROR: file_system_close(): cannot close stdout\n");
        return -1;
    }
    if (fd < 0 || fd > MAX_OPEN_FILE) {
        printf("ERROR: file_system_close(): no such fd!\n");
        return -1;
    }

    ret = opened_files[fd].file_op_table_p->close(fd);

    opened_files[fd].flags = FD_NOT_IN_USE;
    current_open_file_num--;

    return ret;
}

/**
 * Support system call: read(). Call function corresponding to file type
 * @param fd        The file descriptor
 * @param buf       The output buffer
 * @param nbytes    The number to Bytes to read
 * @return the number of Bytes read and placed into buffer , -1 for the bad fd,
 *         0 if offset reach the end of the file
 */
int32_t file_system_read(int32_t fd, void *buf, int32_t nbytes) {
    // check whether the file is opened 
    if (opened_files[fd].flags == FD_NOT_IN_USE) {
        printf("ERROR: file_system_read(): fd %d is not opened\n", fd);
        return -1;
    }

    return opened_files[fd].file_op_table_p->read(fd, buf, nbytes);
}

/**
 * Support system call: write(). Call function corresponding to file type.
 * @param fd        The file descriptor
 * @param buf       The things to write
 * @param nbytes    The number of bytes to write
 * @return 0 for success, -1 for fail
 * @note This is a read only file system, if file type is dir/regular file, return -1
 */
int32_t file_system_write(int32_t fd, const void *buf, int32_t nbytes) {
    // check whether the file is opened 
    if (opened_files[fd].flags == FD_NOT_IN_USE) {
        printf("ERROR: file_system_write(): fd %d is not opened\n", fd);
        return -1;
    }

    return opened_files[fd].file_op_table_p->write(fd, buf, nbytes);
}


/***************************** Public Functions *********************************/

/**
 * Initialize the whole file system, usually called by kernel when init
 * will check whether the system already inited, if not, init the file
 * system and mark it as inited if yes, do nothing and report by print
 * @param fs    The file system image that is loaded as module
 * @return    0 for success, -1 for the file system already inited
 */
int32_t init_file_system(module_t *fs) {
    // check if already inited 
    if (file_system_inited == 1) {
        printf("ERROR: file_system.c: file system already inited\n");
        return -1;
    }

    // Init: set the global variables
    file_system_inited = 1;
    file_system = *fs;

    // store the blocks as global variable 
    boot_block = *((boot_block_t *) fs->mod_start);
    inodes = ((inode_t *) fs->mod_start) + 1;
    data_blocks = ((data_block_t *) fs->mod_start) + boot_block.inode_num + 1;
    
    // Init operation table for terminal
    terminal_op_table.open = terminal_open;
    terminal_op_table.close = terminal_close;
    terminal_op_table.read = terminal_read;
    terminal_op_table.write = terminal_write;

    // Init operation table for RTC
    rtc_op_table.open = local_rtc_open;
    rtc_op_table.close = local_rtc_close;
    rtc_op_table.read = local_rtc_read;
    rtc_op_table.write = local_rtc_write;

    // Init operation table for dir
    dir_op_table.open = dir_open;
    dir_op_table.close = dir_close;
    dir_op_table.read = dir_read;
    dir_op_table.write = dir_write;

    // Init operation table for regular file
    file_op_table.open = file_open;
    file_op_table.close = file_close;
    file_op_table.read = file_read;
    file_op_table.write = file_write;

    // Init PCB

    opened_files[0].file_op_table_p = &terminal_op_table;
    opened_files[0].flags = FD_IN_USE;
    opened_files[0].file_position = opened_files[0].inode = 0;

    opened_files[1].file_op_table_p = &terminal_op_table;
    opened_files[1].flags = FD_IN_USE;
    opened_files[1].file_position = opened_files[1].inode = 1;

    int i;  // loop counter
    for (i = 2; i < MAX_OPEN_FILE; i++) {
        opened_files[i].flags = FD_NOT_IN_USE;
    }


    return 0;
}


/**
 * Find the directory entry in the file system with name: fname and store it into *dentry
 * @param fname    The file name of the entry
 * @param dentry   Pointer of output dentry
 * @return         0 for success, -1 for no such file exist
 */
int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {

    int i;  // loop counter

    // Loop though all the file names
    for (i = 0; i < boot_block.dir_num; i++) {
        // Test whether current file match
        if (!strncmp((int8_t *) fname, (int8_t *) boot_block.dir_entries[i].file_name, FILE_NAME_LENGTH)) {
            // If yes, set dentry and return
            *dentry = boot_block.dir_entries[i];
            return 0;
        }
    }

    // After looping though all dentries and not found, meaning not exist
    return -1;
}

/**
 * Find the directory entry in the file system with index and store it into *dentry
 * @param index     The index of the dentry
 * @param dentry    Pointer of output dentry
 * @return   0 for success, -1 for no such index exist
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry) {

    int i; // loop counter

    // loop though all the file names  
    for (i = 0; i < boot_block.dir_num; i++) {
        // test whether current file match
        if (index == boot_block.dir_entries[i].inode_num) {
            // if yes, set dentry and return
            *dentry = boot_block.dir_entries[i];
            return 0;
        }
    }

    // after looping though all dentries and not found, meaning not exist 
    return -1;
}

/**
 * Read length of bytes starting from the position offset of the given file
 * @param inode     The inode number of the file to be read
 * @param offset    The position begin to be read in the file
 * @param buf       The output buffer
 * @param length    The length to read
 * @return The number of Bytes read and placed into buffer, or
 *         -1 for the bad inode / inode point to bad data block, 0 if offset reach the end of the file
 * @note The caller is responsible for produce enough space for the buffer
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length) {

    // Check whether inode is valid
    if (inode >= boot_block.inode_num) return -1;

    uint32_t file_length = inodes[inode].length_in_bytes;  // the max length of the file

    // Counters, all of them, including offset needed to be updated in the loop
    uint32_t bytes_read = 0; // a counter for how many Bytes have been read

    // Data block
    uint32_t current_data_block_index = offset / FILE_BLOCK_SIZE_IN_BYTES;
    uint32_t current_data_block_num = inodes[inode].data_block_num[current_data_block_index];
    uint32_t current_data_block_offset = offset - (current_data_block_index * FILE_BLOCK_SIZE_IN_BYTES);

    // Check if the data block is valid
    if (current_data_block_num >= boot_block.data_block_num) return -1;

    while (bytes_read < length) {

        // Check if reach the end of the file
        if (offset >= file_length) break;

        // Copy the current Byte
        buf[bytes_read] = data_blocks[current_data_block_num].data[current_data_block_offset];

        // Update the counters
        bytes_read++;
        offset++;
        current_data_block_offset++;

        // Check if need to update block_num
        if (current_data_block_offset >= FILE_BLOCK_SIZE_IN_BYTES) {
            current_data_block_offset -= FILE_BLOCK_SIZE_IN_BYTES;
            current_data_block_index++;
            current_data_block_num = inodes[inode].data_block_num[current_data_block_index];

            // Check if the data block is valid
            if (current_data_block_num >= boot_block.data_block_num) return bytes_read;
        }

    }

    return bytes_read;
}



/**************************** File Operations ****************************/


/**
 * Get a PCB for the file and return its file descriptor number
 * @param filename    The name of the file to open
 * @return The file descriptor (fd) of the file
 */
int32_t file_open(const uint8_t *filename) {

    // Find the correspond dentry
    dentry_t current_dentry;
    if (-1 == read_dentry_by_name(filename, &current_dentry)) {
        printf("WARNING: file_open(): cannot open %s, no such file\n", filename);
    }

    // Get a PCB, guarantee by file_system_open that have space
    int32_t fd = get_free_fd();

    // Init the PCB got
    opened_files[fd].file_op_table_p = &file_op_table;
    opened_files[fd].inode = current_dentry.inode_num;
    opened_files[fd].file_position = 0;  // the beginning of the file
    opened_files[fd].flags = FD_IN_USE;

    return fd;
}

/**
 * Close a file
 * @param fd    The descriptor of the file to be closed
 * @return 0
 */
int32_t file_close(int32_t fd) {
    (void) fd;  // avoid warning
    return 0;
}

/**
 * Read to the end of the file or the end of the buffer whichever occurs sooner
 * @param fd        The file to read
 * @param buf       The output buffer
 * @param nbytes    The size of the buffer
 * @return the number of Bytes read and placed into buffer, or
 *         -1 for the bad inode / inode point to bad data block, 0 if offset reach the end of the file
 */
int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {

    int32_t ret = 0;  // the return value
    uint32_t offset = opened_files[fd].file_position;  // current offset of the file

    // Place the data into buffer
    ret = read_data(opened_files[fd].inode, offset, buf, nbytes);
    // Check if success
    if (ret == -1) return -1;

    // Update the file position
    opened_files[fd].file_position += ret;

    return ret;
}

/**
 * The file system is read only, always return -1 and report error
 */
int32_t file_write(int32_t fd, const void *buf, int32_t nBytes) {
    // Params will not be used, avoid warning
    (void) fd;
    (void) buf;
    (void) nBytes;

    printf("ERROR: file_write(): the file system is read only\n");
    return -1;
}



/**************************** directory operatoins ****************************/

/**
 * Get a PCB for the dir and return its file descriptor number
 * @param filename    The name of the dir to open
 * @return The file descriptor (fd) of the dir
 */
int32_t dir_open(const uint8_t *filename) {
    (void) filename;  // no need to use, avoid warning

    // Get a PCB, garentee by file_system_open that have space
    int32_t fd = get_free_fd();

    // Init the PCB
    opened_files[fd].file_op_table_p = &dir_op_table;
    opened_files[fd].inode = 0;
    opened_files[fd].file_position = 0;  // the beginning of the file
    opened_files[fd].flags = FD_IN_USE;

    return fd;
}

/**
 * Close the dir
 * @param fd    The file descriptor of dir to be closed
 * @return 0
 */
int32_t dir_close(int32_t fd) {
    (void) fd;  // avoid warning
    return 0;
}

/*
 * dir_read()
 * r
 * INPUTS: 
 * int32_t fd: t
 * void* buf: t
 * int32_t nBytes: meaningless 
 * RETURN:
 * FILE_NAME_LENGTH if success 
 * 0 if reach the end 
 * SIDE EFFECTS:
 * the buf will be changed 
 * the file position filed of corresponding PCB will be changed  
 */
/**
 * Read the dentries' names in current dir from recorded file_position, report if reach the end
 * @param fd        File descriptor to the directory
 * @param buf       The output buffer
 * @param nbytes    Maximal size of buf
 * @return The bytes read, the small one of nbytes or FILE_NAME_LENGTH.
 * @note To get the exact length of file name string, use strlen()
 */
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes) {

    // Since only one dir exist, just get from boot block

    // Check if reach the end
    if (opened_files[fd].file_position >= boot_block.dir_num) {
        printf("WARNING: dir_read(): already reach the end\n");
        return 0;
    }

    // Copy the file name into buf
    int32_t buf_size = (nbytes > FILE_NAME_LENGTH ? FILE_NAME_LENGTH : nbytes);
    strncpy(buf, (int8_t *) (boot_block.dir_entries[opened_files[fd].file_position++].file_name),
            buf_size);

    return buf_size;
}

/**
 * The file system is read only, always return -1 and report error
 * @param fd
 * @param buf
 * @param nBytes
 * @return -1
 */
int32_t dir_write(int32_t fd, const void *buf, int32_t nBytes) {

    // Params will not be used, avoid warning
    (void) fd;
    (void) buf;
    (void) nBytes;

    printf("ERROR: dir_write(): the file system is read only\n");
    return -1;
}

/******************************* Extra Support ***************************/

/**
 * Get a PCB for the RTC and return its file descriptor number
 * @param filename    The name of the RTC to open
 * @return The file descriptor (fd) of the RTC
 */
int32_t local_rtc_open(const uint8_t *filename) {

    (void) filename;  // no need to use, avoid warning

    if (rtc_open(filename) != 0) return -1;

    // Get a PCB, guarantee by file_system_open that have space
    int32_t fd = get_free_fd();

    // Init the PCB, guarantee by file_system_open that have space
    opened_files[fd].file_op_table_p = &rtc_op_table;
    opened_files[fd].inode = 0;
    opened_files[fd].file_position = 0;  // the beginning of the file
    opened_files[fd].flags = FD_IN_USE;

    return fd;
}

/**
 * Relay close() to RTC driver
 * @param fd    The file descriptor of RTC
 * @return Return from rtc_close(fd)
 */
int32_t local_rtc_close(int32_t fd) {
    return rtc_close(fd);
}

/**
 * Relay read() to RTC driver
 * @param fd        The file descriptor of RTC
 * @param buf       The buffer to write to
 * @param nbytes    The size of buf
 * @return Return from rtc_read(fd, buf, nbytes)
 */
int32_t local_rtc_read(int32_t fd, void *buf, int32_t nbytes) {
    return rtc_read(fd, buf, nbytes);
}

/**
 * Relay write() to RTC driver
 * @param fd        The file descriptor of RTC
 * @param buf       The buffer to write to
 * @param nbytes    The size of buf
 * @return Return from rtc_write(fd, buf, nbytes)
 */
int32_t local_rtc_write(int32_t fd, const void *buf, int32_t nbytes) {
    return rtc_write(fd, buf, nbytes);
}

/**
 * Get file size directly from inode
 * @param inode    Inode index
 * @return Length in bytes
 */
int32_t get_file_size(uint32_t inode){
    if (inode >= boot_block.inode_num){
        printf("ERROR: get_file_size(): no such inode\n");
        return -1;
    }
    return inodes[inode].length_in_bytes;
}

