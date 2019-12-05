
#include "task_paging.h"

#include "lib.h"
#include "paging.h"
#include "x86_desc.h"

#include "task.h"
#include "file_system.h"

#define     PAGE_ID_USED          666
#define     PAGE_ID_FREE          0

#define     TASK_IMG_LOAD_ADDR    0x08048000

#define     TASK_IMG_PAGE_ENTRY   32          // 128MB / 4MB
#define     TASK_IMG_PAGE_FLAG    0x00000087  // flags for a user level task

#define     ELF_MAGIC_SIZE_IN_BYTE  4

// Global variables
static int page_id_count = 0;  // the ID for new task, also the count of running tasks
static int page_id_running[TASK_MAX_COUNT] = {0};

// Helper functions
static int task_set_img_paging(const int page_id);
static int task_get_free_page_id();
static int task_load(dentry_t *task);
static int task_is_executable(dentry_t *task);
static uint32_t task_get_eip(dentry_t *task);


/**
 * Initialize task paging related things
 */
void task_paging_init() {
    int i;

    // Init task queues
    for (i = 0; i < TASK_MAX_COUNT; i++) {
        // init the global var
        page_id_running[i] = PAGE_ID_FREE;
    }
}

/**
 * Set up paging for a task that is going to run, load program image, and return EIP and page if
 * @param task_name    The name of the task
 * @param eip          The pointer to store eip of the task
 * @return Page ID for success, -1 for no such task, -2 for fail to get eip
 * @effect The paging setting will be changed, arg eip may be set
 */
int task_paging_allocate_and_set(const uint8_t *task_name, uint32_t *eip) {

    int i;  // loop counter and temp use 
    dentry_t task;  // the dentry of the tasks in the file system

    // Check the input 
    if (eip == NULL) {
        DEBUG_ERR("task_paging_allocate_and_set(): NULL eip!");
        return -1;
    }

    // Get the task in file system 
    if (-1 == read_dentry_by_name(task_name, &task)) {
        DEBUG_ERR("task_paging_allocate_and_set(): no such task: %s", task_name);
        return -1;
    }

    // Check whether the file is executable 
    if (!task_is_executable(&task)) {
        DEBUG_ERR("task_paging_allocate_and_set(): not a executable task: %s", task_name);
        return -1;
    }

    // Get a page id for the task
    int page_id = task_get_free_page_id();
    if (page_id == -1) {
        DEBUG_WARN("task_paging_allocate_and_set(): max task reached, cannot open task: %s\n", task_name);
        return -1;
    }

    // Get the eip of the task
    if (-1 == (i = task_get_eip(&task))) {
        DEBUG_ERR("task_paging_allocate_and_set(): fail to get eip of task: %s", task_name);
        return -2;
    } else {
        *eip = i;
    }

    // Turn on the paging space for the file 
    task_set_img_paging(page_id);

    // Load the file
    task_load(&task);

    // Update the page_id 
    page_id_running[page_id] = PAGE_ID_USED;
    page_id_count++;

    return page_id;
}

/**
 * Reset the paging setting when halt a task
 * @param page_id   The page id of the task to halt
 * @return 0 for success , -1 for fail
 */
int task_paging_deallocate(const int page_id) {

    // Check whether the id is valid 
    if (page_id >= TASK_MAX_COUNT) {
        DEBUG_ERR("task_reset_paging(): invalid page id: %d", page_id);
        return -1;
    }
    if (page_id_running[page_id] == PAGE_ID_FREE) {
        DEBUG_ERR("task_reset_paging(): page %d is not allocated", page_id);
        return -1;
    }

    // Does not close user img page

    // Release the page id 
    page_id_running[page_id] = PAGE_ID_FREE;
    page_id_count--;

    return 0;
}

/**
 * Reset the 128-132MB paging for a running task 
 * @param       page_id: the task id
 * @return      0 for success, 1 for failure
 * @effect      the PDE will be changed 
 */
int task_paging_set(const int page_id) {
    if (page_id < 0 || page_id > TASK_MAX_COUNT) {
        DEBUG_ERR("task_paging_set(): bad page id : %d\n", page_id);
        return -1;
    }
    if (page_id_running[page_id] == PAGE_ID_FREE) {
        DEBUG_ERR("task_paging_set(): page %d is not used", page_id);
        return -1;
    }

    task_set_img_paging(page_id);

    return 0;
}


/***************************** Helper Functions *******************************/

/**
 * Turn on the paging for specific task (privilege = 3)
 * Specifically, map 128MB-132MB to start at 8MB + page id * 4MB
 * @param page_id    The page id of the task
 * @return 0
 * @effect The PD will be changed
 */
static int task_set_img_paging(const int page_id) {
    // The PDE to be set 
    uint32_t pde = 0;

    // Calculate the physical memory address, set at bit 22 
    pde |= (((uint32_t) page_id + 2 + KERNEL_PAGE_OFFSET + 1) << 22U);  // +2 for 8MB

    // Set the flags of PDE 
    pde |= TASK_IMG_PAGE_FLAG;

    // Set the PDE 
    kernel_page_directory.entry[TASK_IMG_PAGE_ENTRY] = pde;

    FLUSH_TLB();

    return 0;
}

/**
 * Get a free page id
 * @return The page id got for success , -1 for fail
 */
static int task_get_free_page_id() {
    int page_id = 0;
    for (page_id = 0; page_id < TASK_MAX_COUNT; page_id++) {
        if (page_id_running[page_id] == PAGE_ID_FREE) return page_id;
    }
    return -1;
}

/**************** Executable File Operations ************/

/**
 * Load the whole module at virtual memory TASK_START_MEM (128MB)
 * @param task    the task to be test 
 * @return        0 for not a ELF file, 1 for yes
 * @note          for a ELF file, the first 4 bytes: 0x7F, ELF (in ASCII)
 */
static int task_load(dentry_t *task) {
    uint8_t *task_page_p = (uint8_t *) TASK_IMG_LOAD_ADDR;   // the pointer for loading task
    int i = 0;      // loop counter
    int file_size = get_file_size(task->inode_num);
    int cur_file_inode = task->inode_num;

    read_data(cur_file_inode, i, task_page_p, file_size);

    return 0;
}

/**
 * Test whether the file is a ELF file 
 * @param task    The task to be test
 * @return 0 for not a ELF file, 1 for yes
 * @note For a ELF file, the first 4 bytes: 0x7F, ELF (in ASCII)
 */
static int task_is_executable(dentry_t *task) {

    uint8_t buf[ELF_MAGIC_SIZE_IN_BYTE + 1];  // buffer for storing read
    read_data(task->inode_num, 0, buf, ELF_MAGIC_SIZE_IN_BYTE);

    // Check the magic numbers 
    int ret = 1;
    ret &= (buf[0] == 0x7F);
    ret &= (buf[1] == 'E');
    ret &= (buf[2] == 'L');
    ret &= (buf[3] == 'F');

    return ret;
}

/**
 * Get the EIP of the task in the elf file
 * @param task    The dentry of the elf file
 * @return Its EIP for success, -1 for fail
 * @note The eip is at byte 24~27
 */
static uint32_t task_get_eip(dentry_t *task) {

    uint32_t eip = 0;  // the return value

    // Check if task is NULL
    if (task == NULL) return -1;

    /* Read EIP */
    if (4 != read_data(task->inode_num, 24, (uint8_t *) &eip, 4)) return -1;

    return eip;
}

