
#include "task_paging.h"
#include "file_system.h"
#include "lib.h"
#include "x86_desc.h"

#define ELF_MAGIC_SIZE_IN_BYTE  4
#define TASK_USER_VRAM_START_INDEX    0xB9    // real VRAM is at 0xB8

// Global variables
static int page_id_count = 0; // the ID for new task, also the count of running tasks
static int page_id_running[MAX_RUNNING_TASK] = {0};
static int page_id_terminal[MAX_RUNNING_TASK]; // indicating the termial that the task coorespond to
static int page_id_active;

// video memory 
static int terminal_active;
static int terminal_opened[MAX_NUM_TERMINAL];
static page_table_t user_video_memory_pt[MAX_NUM_TERMINAL] __attribute__((aligned (SIZE_4K)));
static page_table_t kernel_video_memory_pt[MAX_NUM_TERMINAL] __attribute__((aligned (SIZE_4K)));
static int user_video_mapped[MAX_NUM_TERMINAL];

/*
 * Reference for page tables 
 * kernel_page_table_0: PDE 0~4MB 
 *      the page that can access all video memory buffers 
 *      useful for copying between buffers 
 * kernel_page_table_1: PDE 0~4MB 
 *      the page for active terminal, mapping directly to physical VRAM 
 * kernel_video_memory_pt: PDE 0~4MB 
 *      the page for inactive terminal, mapping to its buffer 
 * user_page_table_0: PDE 132~136MB 
 *      the page for user process on active termial, mapping directly to physical VRAM 
 * user_video_memory_pt: PDE 132~136MB 
 *      the page for user process on inactive termial, mapping to its buffer 
 */

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
int task_set_video_map(const int id);

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
 * @param eip          the pointer to store eip of the task 
 * @return             pid for success, -1 for no such task, -2 for fail to get eip 
 * @effect             The paging setting will be changed, para eip may be set 
 */
int task_set_up_memory(const uint8_t *task_name, uint32_t *eip, const int ter_id) {

    int i;  // loop counter and temp use 
    dentry_t task; // the dentry of the tasks in the file system

    // When first run, do some init work
    if (page_id_count == 0) {
        // init task queues 
        for (i = 0; i < MAX_RUNNING_TASK; i++) {
            // init the global var 
            page_id_running[i] = PID_FREE;
            page_id_terminal[i]= TERMINAL_VID_NOT_OPENED ; // indicating not used 
            page_id_active = -1;
        }
        // video memory 
        task_init_video_memory();
        terminal_vid_open(0);
        for (i=0; i<MAX_NUM_TERMINAL; i++){
            terminal_opened[i]=TERMINAL_VID_NOT_OPENED;
        }
    }

    // check the input 
    if (eip == NULL){
        DEBUG_ERR("task_set_up_memory(): NULL eip!\n");
        return -1;
    }
    if (ter_id < 0 || ter_id >= MAX_NUM_TERMINAL){
        DEBUG_ERR("task_set_up_memory(): bad ter_id: %d\n", ter_id);
        return -1;
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
    page_id_terminal[page_id] = ter_id;
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

    // Close the user video memory map 
    /* Note: assuming the pre_id task didn't call vidmap, which is always the case 
     * The tasks that call vidmap never execute other task on it 
     * For now, TASK_VRAM_MAPPED is only useful for restoring when switching termial 
     * To improve: remember each task: whether map/ which termial 
     */
    task_set_video_map(-2);
    user_video_mapped[terminal_active]= TASK_VRAM_NOT_MAPPED;


    // Release the pid 
    page_id_running[cur_id] = PID_FREE;
    page_id_terminal[cur_id] = TERMINAL_VID_NOT_OPENED;
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
int task_vidmap(uint8_t ** screen_start){
    
    // Check whether to dest to write is valid 
    if ((int)screen_start < TASK_START_MEM || (int)screen_start >= TASK_END_MEM){
        DEBUG_ERR("screen_start out of range: %x\n", (int)screen_start);
        return -1;
    }

    // check whether the current task ter_id correct 
    if (page_id_terminal[page_id_active] < 0 || page_id_terminal[page_id_active] >= MAX_NUM_TERMINAL){
        DEBUG_ERR("task_get_vidmap(): HUGE MISTAKE! task's termial not set!\n");
        return -1;
    }

    // Map the page according to task's termial
    if (page_id_terminal[page_id_active] == terminal_active){
        // Set to active
        if (-1 ==task_set_video_map(-1) ) return -1;  
    } else {
        // Set to corresponding buf
        if (-1 == task_set_video_map(page_id_terminal[page_id_active]) ) return -1; 
    }
    user_video_mapped[terminal_active] = TASK_VRAM_MAPPED;

    *screen_start = (uint8_t*)(TASK_END_MEM + TASK_VIR_VIDEO_MEM_ENTRY * SIZE_4K);

    return 0;
}

/************************* Active termial operations ***************************/

/**
 * Open a video memory that is not yet opened 
 * Set up the paging maping for it 
 * But not set it to active 
 * @param ter_id    the termial to be open , 0 <= ter_id < MAX_NUM_TERMINAL
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed
 */
int terminal_vid_open(const int ter_id){
    if (ter_id < 0 || ter_id >= MAX_NUM_TERMINAL){
        DEBUG_ERR("terminal_vid_open(): no such ter_id: %d\n", ter_id);
        return -1;
    }
    if (terminal_opened[ter_id]==TERMINAL_VID_OPENED){
        DEBUG_WARN("terminal_vid_open(): already opened ter_id: %d\n", ter_id);
    }

    terminal_opened[ter_id]=TERMINAL_VID_OPENED;

    return 0;
}

/**
 * Close a terminal video memory that is opened 
 * IMPORTANT: the termial to close should not be active 
 * if want to close a termial, please call switch first 
 * @param ter_id    the termial to be open , 0 <= ter_id < MAX_NUM_TERMINAL
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed
 */
int terminal_vid_close(const int ter_id){
    if (ter_id < 0 || ter_id >= MAX_NUM_TERMINAL){
        DEBUG_ERR("terminal_vid_close(): no such ter_id: %d\n", ter_id);
        return -1;
    }
    if (ter_id == terminal_active){
        DEBUG_ERR("terminal_vid_close(): cannot close active ter_id: %d\n", ter_id);
        return -1;
    }
    if (terminal_opened[ter_id]==TERMINAL_VID_NOT_OPENED){
        DEBUG_WARN("terminal_vid_open(): already closed ter_id: %d\n", ter_id);
    }

    terminal_opened[ter_id]=TERMINAL_VID_NOT_OPENED;

    return 0;
}

/**
 * Set a opened termial video memory to be active, i.e. change the screen 
 * and store the old one to its buffer 
 * @param cur_ter_id    the termial to be set active 
 * @param pre_ter_id    the termial to be stored, or MAGIC_NO_TERMINAL_OPENED 
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed, the screen will be changed 
 */
int terminal_active_vid_switch(const int cur_ter_id, const int pre_ter_id){
    // Error checking 

    // Turn on the kernel page for copy 

    // Copy the previous terminal VRAM from physical VRAM to its buffer 

    // Copy the current terminal VRAM from its buffer to physical VRAM

    // Turn on active kernel paging 
    
    // Set global variables 
    
    FLUSH_TLB();
    return 0;
}

/**
 * Set current paging pointing VRAM to buffer for inactive terminal 
 * @param ter_id    the termial to be set 
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed
 */
int terminal_vid_set(const int ter_id){
    // Error checking
    if (ter_id < -2 || ter_id >= MAX_NUM_TERMINAL){
        DEBUG_ERR("terminal_vid_set(): bad ter_id: %d\n", ter_id);
        return -1;
    }
    if (terminal_opened[ter_id] == TERMINAL_VID_NOT_OPENED){
        DEBUG_ERR("terminal_vid_set(): unopened ter_id: %d\n", ter_id);
        return -1;
    }

    // Get the PDE for 0~4MB 
    PDE_4kB_t* kernel_vram_pde = NULL;
    kernel_vram_pde = (PDE_4kB_t*)(&kernel_page_directory.entry[0]);
    if (-1 == task_clear_PDE_4kB(kernel_vram_pde)) return -1;

    // set it pointing to corresponding PT 
    kernel_page_directory.entry[0] |= (uint32_t) &kernel_video_memory_pt[ter_id];
    kernel_vram_pde->can_write = 1;
    kernel_vram_pde->present = 1;

    FLUSH_TLB();
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
 * 1. (deleted) Set the PDE for 132MB-136MB to be 4kB page, map it to kernel 0-4MB initially
 * 2. Set global variables of PT for each task's video memory, map 0xB8 to its VRAM
 * 3. Set (0xB9+pid)*4kB in the 0-4MB page to be present
 * @return      0 for success, -1 for fail
 * @effect      PDE, PTE will be changed. TLB will be flushed 
 */
int task_init_video_memory(){
    
    int i=0; // loop counter 
    int j=0; // loop counter 
    PTE_t* cur_pte = NULL; 
    
    // Set global variables of PT for each termial's video memory 
    // Set (0xB9+pid)*4kB in the 0-4MB page to be present
    for (i=0; i< MAX_NUM_TERMINAL; i++){
        
        // Clear the PTE 
        for (j=0; j<SIZE_K; j++){
            cur_pte = (PTE_t*)(&user_video_memory_pt[i].entry[j]);
            task_clear_PTE(cur_pte);
            cur_pte = (PTE_t*)(&kernel_video_memory_pt[i].entry[j]);
            task_clear_PTE(cur_pte);
        }

        // User video memory page 
        // Get PTE at 0xB8 
        cur_pte = (PTE_t*)(&user_video_memory_pt[i].entry[TASK_VIR_VIDEO_MEM_ENTRY]);
        // Map the PTE to corresponding page at (0xB9+pid)*4kB
        cur_pte->base_address = (TASK_VIR_VIDEO_MEM_ENTRY + i) * SIZE_4K;
        cur_pte->can_write = 1;
        cur_pte->present = 1;
        cur_pte->user_or_super = 1; // user

        // Kernel video memory page 
        // Get PTE at 0xB8 
        cur_pte = (PTE_t*)(&kernel_video_memory_pt[i].entry[TASK_VIR_VIDEO_MEM_ENTRY]);
        // Map the PTE to corresponding page at (0xB9+pid)*4kB
        cur_pte->base_address = (TASK_VIR_VIDEO_MEM_ENTRY + i) * SIZE_4K;
        cur_pte->can_write = 1;
        cur_pte->present = 1;
        cur_pte->user_or_super = 0; // kernel 

        // update 0~4MB kernel page 
        PTE_t* cur_kernel_pte = NULL;
        cur_kernel_pte= (PTE_t*)(&kernel_page_table_0.entry[TASK_VIR_VIDEO_MEM_ENTRY + i]);
        *cur_kernel_pte=*cur_pte;

        // update map flag array 
        user_video_mapped[i]=TASK_VRAM_NOT_MAPPED;
        
    }


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


/**
 * Set the PDE for 132MB-136MB to be 4kB page, pointing to corresponding PT 
 * The active PT map directly to physical VRAM
 * @param       id: the id of the inactive termial VRAM buffer, -1 indicates active, -2 indicates close
 * @return      0 for success, -1 for bad id 
 * @effect      PDE will be changed 
 */
int task_set_video_map(const int id){
    if (id < -2 || id >= MAX_NUM_TERMINAL){
        DEBUG_ERR("task_set_video_map(): bad id: %d\n", id);
        return -1;
    }
    
    // Get the PDE for 132~136MB
    PDE_4kB_t* user_vram_pde = NULL;
    user_vram_pde = (PDE_4kB_t*)(&kernel_page_directory.entry[TASK_VIR_MEM_ENTRY+1]);
    // Clear to PDE 
    if (-1 == task_clear_PDE_4kB(user_vram_pde)) return -1;

    // finish if closing the page 
    if (id == -2) return -1;

    // Set the fields (for active id (-1), physical VRAM)
    if (id == -1) {
        kernel_page_directory.entry[TASK_VIR_MEM_ENTRY+1] |= (uint32_t) &user_page_table_0;
    } else {
        kernel_page_directory.entry[TASK_VIR_MEM_ENTRY+1] |= (uint32_t) &user_video_memory_pt[id];
    }

    user_vram_pde->can_write = 1;
    user_vram_pde->present = 1;
    user_vram_pde->user_or_super = 1;

    FLUSH_TLB();
    return 0;
}

int terminal_copy_from_physical(const int dest_id){
    return 0;
}

int terminal_copy_to_physcial(const int src_id){
    return 0;
}
