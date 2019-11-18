
#include "task_paging.h"
#include "file_system.h"
#include "lib.h"
#include "x86_desc.h"

#define ELF_MAGIC_SIZE_IN_BYTE  4
#define TASK_USER_VRAM_START_INDEX    0xB9    // real VRAM is at 0xB8

// Global variables
static int page_id_count = 0; // the ID for new task, also the count of running tasks
static int page_id_running[MAX_RUNNING_TASK] = {0};
static int page_id_active;

// video memory 
static kernel_page_table_t user_video_memory_PT[MAX_RUNNING_TASK];


// Helper functions
int task_turn_on_paging(const int id);
int task_turn_off_paging(const int id, const int pre_id);

int task_load(dentry_t *task);
int task_is_executable(dentry_t *task);
uint32_t task_get_eip(dentry_t *task);
int task_get_page_id();

int task_init_video_memory();
int task_clear_PDE_4MB(PDE_4MB_t* entry);
int task_clear_PDE_4kB(PDE_4kB_t* entry); 
int task_clear_PTE(PTE_t* entry);

#define FLUSH_TLB()  asm volatile ("  \
    movl    %%cr3, %%eax            \n\
    movl    %%eax, %%cr3"             \
    : \
    : \
    : "cc", "memory", "eax")

#define SET_PDBR(addr) asm volatile ("  \
    movl    %0, %%cr3 "                 \
    : /* no outputs*/                   \
    : /* inputs: */  "d" ((addr))       \
    : /* flags: */  "cc", "memory" ) 


/**
 * Set up paging for a task that is going to run and get its eip
 * @param task_name    the name of the task
 * @return             pid for success, -1 for no such task, -2 for fail to get eip 
 * @effect             The paging setting will be changed, para eip may be set 
 */
int task_set_up_memory(const uint8_t *task_name, uint32_t *eip) {

    int i;  // loop counter and temp use 
    dentry_t task; // the dentry of the tasks in the file system

    // When first run, do some init work
    if (page_id_count == 0) {
        for (i = 0; i < MAX_RUNNING_TASK; i++) {
            // init the global var 
            page_id_running[i] = PID_FREE;
            page_id_active = -1;
            // init pages for in-active video memory
            task_init_video_memory();
        }
    }

    // Get the task in file system 
    if (-1 == read_dentry_by_name(task_name, &task)) {
        DEBUG_ERR("task_set_up_memory(): no such task: %s\n", task_name);
        return -1;
    }

    // Check whether the file is executable 
    if (!task_is_executable(&task)) {
        DEBUG_ERR("task_set_up_memory(): not a executable task: %s\n", task_name);
        return -1;
    }

    // Get a pid for the task
    int page_id = task_get_page_id();
    if (page_id == -1) {
        DEBUG_WARN("task_set_up_memory(): max task reached, cannot open task: %s\n", task_name);
        return -1;
    }

    // Get the eip of the task
    if (-1 == (i = task_get_eip(&task))) {
        DEBUG_ERR("task_set_up_memory(): fail to get eip of task: %s\n", task_name);
        return -2;
    } else {
        *eip = i;
    }

    // Turn on the paging space for the file 
    task_turn_on_paging(page_id);

    // Load the file
    task_load(&task);

    // Update the pid 
    page_id_running[page_id] = PID_USED;
    page_id_active = page_id;
    page_id_count++;

    return page_id;
}

/**
 * Reset the paging setting when halt a task 
 * Should only called on the latest running task
 * @param id    the id of the task
 * @return      0 for success , -1 for fail
 * @effect      The paging setting will be changed 
 */
int task_reset_paging(const int cur_id, const int pre_id) {

    // Check whether the id is valid 
    if (cur_id >= MAX_RUNNING_TASK) {
        DEBUG_ERR("task_reset_paging(): invalid task id: %d\n", cur_id);
        return -1;
    }
    if (page_id_running[cur_id] == PID_FREE) {
        DEBUG_ERR("task_reset_paging(): task %d is not running\n", cur_id);
        return -1;
    }

    // Turn off the paging for cur_id, replace it with pre_id 
    task_turn_off_paging(cur_id, pre_id);

    // Release the pid 
    page_id_running[cur_id] = PID_FREE;
    page_id_active = pre_id;
    page_id_count--;

    return 0;
}


/**
 * Map current task's virtual VRAM to kernel VRAM 
 * Write the task's VRAM pointer to *screen_start
 * @return      0 for success, -1 for fail
 * @effect      *screen_start will be changed
 */
int task_get_vimap(uint8_t ** screen_start){
    
    // Check whether to dest to write is valid 
    if ((int)screen_start < TASK_START_MEM || (int)screen_start >= TASK_END_MEM){
        DEBUG_ERR("screen_start out of range: %d\n", (int)screen_start);
        return -1;
    }

    *screen_start = (uint8_t*)(TASK_END_MEM + 0xB8000);

    // Map the page according to current pid 
    (void)page_id_active;
    return 0;
}

/***************************** Helper Functions *******************************/

/**
 * Turn on the paging for specific task (privilege = 3)
 * Specificly, map 128MB~132MB to start at 8MB + pid*(4MB) 
 * @param id    the pid of the task
 * @return      always 0 
 * @effect      The PD will be changed 
 */
int task_turn_on_paging(const int id) {
    // The PDE to be set 
    uint32_t pde = 0;

    // Calculate the physical memory address, set at bit 22 
    pde |= (((uint32_t) id + 2) << 22U);  // +2 for 8MB

    // Set the flags of PDE 
    pde |= TASK_PAGE_FLAG;

    // Set the PDE 
    kernel_page_directory.entry[TASK_VIR_MEM_ENTRY] = pde;

    FLUSH_TLB();

    return 0;
}

/**
 * Turn off the paging for specific task 
 * and reset the paging for previous task 
 * @param cur_id    the pid of the task
 * @param pre_id    the pid of the previous task
 * @return          always 0 
 * @effect          The PD will be changed 
 */
int task_turn_off_paging(const int cur_id, const int pre_id) {
    (void) cur_id;  // seems not useful, avoid warning
    return task_turn_on_paging(pre_id);
}

/**
 * Load the whole module at virtual memory TASK_START_MEM (128MB)
 * @param task    the task to be test 
 * @return        0 for not a ELF file, 1 for yes
 * @note          for a ELF file, the first 4 bytes: 0x7F, ELF (in ASCII)
 */
int task_load(dentry_t *task) {
    uint8_t *task_page_p = (uint8_t *) TASK_IMG_LOAD_ADDR;   // the pointer for loading task
    uint8_t buf;     // the buffer for reading one byte
    int i = 0;      // loop counter
    int file_size = get_file_size(task->inode_num);
    int cur_file_inode = task->inode_num;

    for (i = 0; i < file_size; i++) {
        // Read the current byte 
        read_data(cur_file_inode, i, &buf, 1);
        // Copy the current byte into memory 
        *(task_page_p + i) = buf;
    }
    // TODO: why not directly write to task_page_p with length file_size?

    return 0;
}

/**
 * Test whether the file is a ELF file 
 * @param task    the task to be test 
 * @return        0 for not a ELF file, 1 for yes
 * @note          for a ELF file, the first 4 bytes: 0x7F, ELF (in ASCII)
 */
int task_is_executable(dentry_t *task) {

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
 * Get the eip of the task in the elf file 
 * @param task    the dentry of the elf file 
 * @return        its pid for success, -1 for fail 
 * @note          the eip is at byte 24~27
 */
uint32_t task_get_eip(dentry_t *task) {

    uint32_t eip = 0;  // the return value

    // Check if task is NULL
    if (task == NULL) return -1;

    /* Read EIP */
    if (4 != read_data(task->inode_num, 24, (uint8_t *) &eip, 4)) return -1;

    return eip;
}

/**
 * Get a free pid 
 * @return      the pid got for success , -1 for fail
 */
int task_get_page_id() {
    int page_id = 0;
    for (page_id = 0; page_id < MAX_RUNNING_TASK; page_id++) {
        if (page_id_running[page_id] == PID_FREE) return page_id;
    }
    return -1;
}

/**
 * Initialize the pages for holding user video memory 
 * 1. Set the PDE for 132MB-136MB to be 4kB page, map it to kernel 0-4MB initially
 * 2. Set global variables of PT for each task's video memory, map 0xB8 to its VRAM
 * 3. Set (0xB9+pid)*4kB in the 0-4MB page to be present
 * @return      0 for success, -1 for fail
 * @effect      PDE, PTE will be changed. TLB will be flushed 
 */
int task_init_video_memory(){
    
    // Set the PDE for 132MB-136MB to be 4kB page, pointing to a PTE
    // Get the PDE 
    PDE_4kB_t* user_vram_pde = NULL;
    user_vram_pde = (PDE_4kB_t*)(&kernel_page_directory.entry[TASK_VIR_MEM_ENTRY+1]);
    // Clear to PDE 
    if (-1 == task_clear_PDE_4kB(user_vram_pde)) return -1;
    // Set the fields (initially, map to kernel page table 0~4MB)
    kernel_page_directory.entry[TASK_VIR_MEM_ENTRY+1] |= (uint32_t)kernel_page_table_0.entry;
    user_vram_pde->can_write = 1;
    user_vram_pde->present = 1;
    // set to user level ???
    

    // Set global variables of PT for each task's video memory 
    // Get PTE at 0xB8 

    // Clear the PTE 

    // Map the PTE to corresponding page at (0xB9+pid)*4kB

    
    // Set (0xB9+pid)*4kB in the 0-4MB page to be present


    FLUSH_TLB();
    return 0;
}

/**
 * Clear a 4MB PDE by setting all the flags to 0 and address to 0 
 * Then set page_size flag to 1
 * @return      0 for success, -1 for fail
 * @effect      *entry will be changed 
 */
int task_clear_PDE_4MB(PDE_4MB_t* entry){
    if (entry == NULL ){
        DEBUG_ERR("task_clear_PDE_4MB(): bad input!\n");
        return -1;
    }

    entry->base_address = 0;
    entry->pat = 0;
    entry->available = 0; 
    entry->global = 0;
    entry->page_size = 1; // 4MB
    entry->dirty = 0;
    entry->accessed = 0;
    entry->cache_disabled = 0;
    entry->write_through = 0;
    entry->user_or_super = 0;
    entry->can_write = 0;
    entry->present = 0;

    return 0;
}

/**
 * Clear a 4kB PDE by setting all the flags to 0 and address to 0 
 * @return      0 for success, -1 for fail
 * @effect      *entry will be changed 
 */
int task_clear_PDE_4kB(PDE_4kB_t* entry){
    if (entry == NULL ){
        DEBUG_ERR("task_clear_PDE_4MB(): bad input!\n");
        return -1;
    }

    entry->base_address = 0;
    entry->global = 0;
    entry->page_size = 0; // 4kB
    entry->accessed = 0;
    entry->cache_disabled = 0;
    entry->write_through = 0;
    entry->user_or_super = 0;
    entry->can_write = 0;
    entry->present = 0;

    return 0;
}

/**
 * Clear a 4MB PTE by setting all the flags to 0 and address to 0 
 * @return      0 for success, -1 for fail
 * @effect      *entry will be changed 
 */
int task_clear_PTE(PTE_t* entry){
    if (entry == NULL ){
        DEBUG_ERR("task_clear_PDE_4MB(): bad input!\n");
        return -1;
    }

    entry->base_address = 0;
    entry->global = 0;
    entry->pat = 0;
    entry->dirty = 0;
    entry->accessed = 0;
    entry->cache_disabled = 0;
    entry->write_through = 0;
    entry->user_or_super = 0;
    entry->can_write = 0;
    entry->present = 0;

    return 0;
}
