
#include "task_paging.h"


#include "lib.h"
#include "x86_desc.h"
#include "file_system.h"
#include "task.h"
#include "terminal.h"

#define     PID_USED            666  // good number
#define     PID_FREE            0

#define     TASK_VRAM_MAPPED        88  // interesting number
#define     TASK_VRAM_NOT_MAPPED    0

#define     TASK_START_MEM      0x08000000  // 128MB
#define     TASK_END_MEM        0x08400000  // 132MB
#define     TASK_IMG_LOAD_ADDR  0x08048000
#define     TASK_PAGE_FLAG      0x00000087  // flags for a user level task
#define     TASK_VIR_MEM_ENTRY  32          // 128MB / 4MB
#define     TASK_VIR_VIDEO_MEM_ENTRY    0xB8

#define     TERMINAL_VID_OPENED         777     // funny number
#define     TERMINAL_VID_NOT_OPENED     0

#define ELF_MAGIC_SIZE_IN_BYTE  4
#define TASK_USER_VRAM_START_INDEX    0xB9    // real VRAM is at 0xB8

// Global variables
static int page_id_count = 0; // the ID for new task, also the count of running tasks
static int page_id_running[TASK_MAX_COUNT] = {0};
static int page_id_terminal[TASK_MAX_COUNT]; // indicating the terminal that the task correspond to
static int page_id_active;

// Video memory 
static int terminal_active;
static int terminal_opened[TERMINAL_MAX_COUNT];
static page_table_t user_video_memory_pt[TERMINAL_MAX_COUNT] ;
static page_table_t kernel_video_memory_pt[TERMINAL_MAX_COUNT];
static int user_video_mapped[TERMINAL_MAX_COUNT];


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
 *      the page for user process on active terminal, mapping directly to physical VRAM
 * user_video_memory_pt: PDE 132~136MB 
 *      the page for user process on inactive terminal, mapping to its buffer
 */

// Helper functions
int task_turn_on_paging(const int id);

int task_turn_off_paging(const int id, const int pre_id);

int task_get_page_id();

int task_load(dentry_t *task);

int task_is_executable(dentry_t *task);

uint32_t task_get_eip(dentry_t *task);

int task_init_video_memory();

int task_set_user_video_map(const int id);

int terminal_copy_from_physical(const int dest_id);

int terminal_copy_to_physical(const int src_id);

int clear_PDE_4MB(PDE_4MB_t *entry);

int clear_PDE_4kB(PDE_4kB_t *entry);

int clear_PTE(PTE_t *entry);

int set_PDE_4kB(PDE_4kB_t *pde, uint32_t pt, uint8_t can_write, uint8_t user, uint8_t present);

int set_PTE(PTE_t *pte, uint32_t page, uint8_t can_write, uint8_t user, uint8_t present);

int is_valid_flag(uint8_t flag);

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
 * Initialize task paging related things
 */
void task_paging_init() {
    int i;  // loop counter and temp use

    // Init task queues
    for (i = 0; i < TASK_MAX_COUNT; i++) {
        // init the global var
        page_id_running[i] = PID_FREE;
        page_id_terminal[i] = TERMINAL_VID_NOT_OPENED; // indicating not used
        page_id_active = -1;
    }
    // Video memory
    task_init_video_memory();
    for (i = 0; i < TERMINAL_MAX_COUNT; i++) {
        terminal_opened[i] = TERMINAL_VID_NOT_OPENED;
    }
}

/**
 * Set up paging for a task that is going to run and get its eip
 * @param task_name    The name of the task
 * @param eip          The pointer to store eip of the task
 * @return             Page ID for success, -1 for no such task, -2 for fail to get eip
 * @effect             The paging setting will be changed, arg eip may be set
 */
int task_set_up_memory(const uint8_t *task_name, uint32_t *eip, const int ter_id) {

    int i;  // loop counter and temp use 
    dentry_t task;  // the dentry of the tasks in the file system

    // Check the input 
    if (eip == NULL) {
        DEBUG_ERR("task_set_up_memory(): NULL eip!");
        return -1;
    }
    if (ter_id < 0 || ter_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("task_set_up_memory(): bad ter_id: %d", ter_id);
        return -1;
    }

    // Get the task in file system 
    if (-1 == read_dentry_by_name(task_name, &task)) {
        DEBUG_ERR("task_set_up_memory(): no such task: %s", task_name);
        return -1;
    }

    // Check whether the file is executable 
    if (!task_is_executable(&task)) {
        DEBUG_ERR("task_set_up_memory(): not a executable task: %s", task_name);
        return -1;
    }

    // Get a page id for the task
    int page_id = task_get_page_id();
    if (page_id == -1) {
        DEBUG_WARN("task_set_up_memory(): max task reached, cannot open task: %s\n", task_name);
        return -1;
    }

    // Get the eip of the task
    if (-1 == (i = task_get_eip(&task))) {
        DEBUG_ERR("task_set_up_memory(): fail to get eip of task: %s", task_name);
        return -2;
    } else {
        *eip = i;
    }

    // Turn on the paging space for the file 
    task_turn_on_paging(page_id);

    // Load the file
    task_load(&task);

    // Update the page_id 
    page_id_running[page_id] = PID_USED;
    page_id_terminal[page_id] = ter_id;
    page_id_active = page_id;
    page_id_count++;

    return page_id;
}

/**
 * Reset the paging setting when halt a task
 * @param cur_id    The page id of the task to halt
 * @param pre_id    The page id of its parent
 * @return 0 for success , -1 for fail
 * @effect The paging setting will be changed
 * @note Now this function assume the parent didn't call vidmap(). See comment below.
 */
int task_reset_paging(const int cur_id, const int pre_id) {

    // Check whether the id is valid 
    if (cur_id >= TASK_MAX_COUNT) {
        DEBUG_ERR("task_reset_paging(): invalid task id: %d", cur_id);
        return -1;
    }
    if (page_id_running[cur_id] == PID_FREE) {
        DEBUG_ERR("task_reset_paging(): task %d is not running", cur_id);
        return -1;
    }

    // Turn off the paging for cur_id, replace it with pre_id 
    task_turn_off_paging(cur_id, pre_id);

    // Close the user video memory map

    /* Note: assuming the pre_id task didn't call vidmap, which is always the case 
     * The tasks that call vidmap never execute other task on it 
     * For now, TASK_VRAM_MAPPED is only useful for restoring when switching terminal
     * To improve: remember each task: whether map / which terminal
     */
    task_set_user_video_map(-2);
    user_video_mapped[terminal_active] = TASK_VRAM_NOT_MAPPED;


    // Release the page id 
    page_id_running[cur_id] = PID_FREE;
    page_id_terminal[cur_id] = TERMINAL_VID_NOT_OPENED;
    page_id_active = pre_id;
    page_id_count--;

    return 0;
}

/**
 * Reset the 128-132MB paging for a running task 
 * @param       id: the task id 
 * @return      0 for success, 1 for failure
 * @effect      the PDE will be changed 
 */
int task_running_paging_set(const int id){
    if (id < 0 || id > TASK_MAX_COUNT){
        DEBUG_ERR("task_running_paging_set(): bad id : %d\n", id);
        return -1;
    }
    if (page_id_running[id] == PID_FREE){
        DEBUG_ERR("task_running_paging_set(): task %d is not running", id);
        return -1;
    }

    task_turn_on_paging(id);
    return 0;
}


/**
 * Map current task's virtual VRAM to kernel VRAM 
 * Write the task's VRAM pointer to *screen_start
 * @return      0 for success, -1 for fail
 * @effect      *screen_start will be changed
 */
int system_vidmap(uint8_t **screen_start) {

    // Check whether to dest to write is valid 
    if ((int) screen_start < TASK_START_MEM || (int) screen_start >= TASK_END_MEM) {
        DEBUG_ERR("screen_start out of range: %x", (int) screen_start);
        return -1;
    }

    // Check whether the current task ter_id correct
    if (page_id_terminal[page_id_active] < 0 || page_id_terminal[page_id_active] >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("task_get_vidmap(): HUGE MISTAKE! task's terminal not set!");
        return -1;
    }

    // Map the page according to task's terminal
    if (page_id_terminal[page_id_active] == terminal_active) {
        // Set to active
        if (-1 == task_set_user_video_map(-1)) return -1;
    } else {
        // Set to corresponding buf
        if (-1 == task_set_user_video_map(page_id_terminal[page_id_active])) return -1;
    }
    user_video_mapped[terminal_active] = TASK_VRAM_MAPPED;

    *screen_start = (uint8_t *) (TASK_END_MEM + TASK_VIR_VIDEO_MEM_ENTRY * SIZE_4K);

    return 0;
}

/************************* Active Terminal Operations ***************************/

/**
 * Open a video memory that is not yet opened 
 * Set up the paging mapping for it
 * But not set it to active 
 * @param ter_id    the terminal to be open , 0 <= ter_id < TERMINAL_MAX_COUNT
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed
 */
int terminal_vid_open(const int ter_id) {
    if (ter_id < 0 || ter_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_vid_open(): no such ter_id: %d", ter_id);
        return -1;
    }
    if (terminal_opened[ter_id] == TERMINAL_VID_OPENED) {
        DEBUG_WARN("terminal_vid_open(): already opened ter_id: %d\n", ter_id);
    }

    terminal_opened[ter_id] = TERMINAL_VID_OPENED;

    return 0;
}

/**
 * Close a terminal video memory that is opened 
 * IMPORTANT: the terminal to close should not be active
 * if want to close a terminal, please call switch first
 * @param ter_id    the terminal to be open , 0 <= ter_id < TERMINAL_MAX_COUNT
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed
 */
int terminal_vid_close(const int ter_id) {
    if (ter_id < 0 || ter_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_vid_close(): no such ter_id: %d", ter_id);
        return -1;
    }
    if (ter_id == terminal_active) {
        DEBUG_ERR("terminal_vid_close(): cannot close active ter_id: %d", ter_id);
        return -1;
    }
    if (terminal_opened[ter_id] == TERMINAL_VID_NOT_OPENED) {
        DEBUG_WARN("terminal_vid_open(): already closed ter_id: %d\n", ter_id);
    }

    terminal_opened[ter_id] = TERMINAL_VID_NOT_OPENED;

    return 0;
}

/**
 * Set a opened terminal video memory to be active, i.e. change the screen
 * and store the old one to its buffer 
 * @param new_ter_id    the terminal to be set active
 * @param pre_ter_id    the terminal to be stored, or NULL_TERMINAL_ID
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed, the screen will be changed 
 */
int terminal_active_vid_switch(const int new_ter_id, const int pre_ter_id) {

    // Error checking
    // Check new_ter_id
    if (new_ter_id < 0 || new_ter_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_active_vid_switch(): bad new_ter_id: %d", new_ter_id);
        return -1;
    }
    if (terminal_opened[new_ter_id] == TERMINAL_VID_NOT_OPENED) {
        DEBUG_ERR("terminal_active_vid_switch(): not opened new_ter_id: %d", new_ter_id);
        return -1;
    }
    // Check pre_ter_id
    if ((pre_ter_id < 0 || pre_ter_id >= TERMINAL_MAX_COUNT) && pre_ter_id != NULL_TERMINAL_ID) {
        DEBUG_ERR("terminal_active_vid_switch(): bad pre_ter_id: %d", pre_ter_id);
        return -1;
    }
    if (pre_ter_id != NULL_TERMINAL_ID && terminal_opened[pre_ter_id] == TERMINAL_VID_NOT_OPENED) {
        DEBUG_ERR("terminal_active_vid_switch(): not opened pre_ter_id: %d", new_ter_id);
        return -1;
    }

    if (new_ter_id == pre_ter_id) return 0;

    // Turn on the kernel page for copy
    if (-1 == set_PDE_4kB((PDE_4kB_t *) (&kernel_page_directory.entry[0]),
                          (uint32_t) &kernel_page_table_0, 1, 0, 1)) {
        return -1;
    }
    FLUSH_TLB();

//    printf("%d -> %d\n", pre_ter_id, new_ter_id);

    if (pre_ter_id != NULL_TERMINAL_ID) {
        // Copy the previous terminal VRAM from physical VRAM to its buffer
        terminal_copy_from_physical(pre_ter_id);
    }

    // Copy the current terminal VRAM from its buffer to physical VRAM
    terminal_copy_to_physical(new_ter_id);

    // Turn on active kernel paging
    if (-1 == set_PDE_4kB((PDE_4kB_t *) (&kernel_page_directory.entry[0]),
                          (uint32_t) &kernel_page_table_1, 1, 0, 1)) {
        return -1;
    }
    FLUSH_TLB();

    // Set user vidmap if necessary
    if (user_video_mapped[new_ter_id] == TASK_VRAM_MAPPED) {
        task_set_user_video_map(-1);
    }

    // Set global variables 
    terminal_active = new_ter_id;

    return 0;
}

/**
 * Set current paging pointing VRAM to buffer for running terminal 
 * @param ter_id    the terminal to be set
 * @return          0 for success, -1 for fail 
 * @effect          PDE will be changed
 */
int terminal_vid_set(const int ter_id) {

    // Error checking
    if (ter_id < -2 || ter_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_vid_set(): bad ter_id: %d", ter_id);
        return -1;
    }
    if (terminal_opened[ter_id] == TERMINAL_VID_NOT_OPENED) {
        DEBUG_ERR("terminal_vid_set(): unopened ter_id: %d", ter_id);
        return -1;
    }

    // Set the PDE for 0-4MB pointing to corresponding PT
    if (ter_id == terminal_active) {
        if (-1 == set_PDE_4kB((PDE_4kB_t *) (&kernel_page_directory.entry[0]),
                              (uint32_t) &kernel_page_table_1, 1, 0, 1)) {
            return -1;
        }
    } else {
        if (-1 == set_PDE_4kB((PDE_4kB_t *) (&kernel_page_directory.entry[0]),
                              (uint32_t) &kernel_video_memory_pt[ter_id], 1, 0, 1)) {
            return -1;
        }
    }

    // Set user vidmap if necessary
    if (user_video_mapped[ter_id] == TASK_VRAM_MAPPED) {
        if (ter_id == terminal_active) {
            task_set_user_video_map(-1);
        } else {
            task_set_user_video_map(ter_id);
        }
    }

    FLUSH_TLB();
    return 0;
}


/***************************** Helper Functions *******************************/


/**
 * Turn on the paging for specific task (privilege = 3)
 * Specifically, map 128MB~132MB to start at 8MB + pid*(4MB)
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
 * Get a free pid 
 * @return      the pid got for success , -1 for fail
 */
int task_get_page_id() {
    int page_id = 0;
    for (page_id = 0; page_id < TASK_MAX_COUNT; page_id++) {
        if (page_id_running[page_id] == PID_FREE) return page_id;
    }
    return -1;
}

/**************** executable file operations ************/

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

/**************** video memory operations ************/

/**
 * Initialize the pages for holding user video memory 
 * 1. (deleted) Set the PDE for 132MB-136MB to be 4kB page, map it to kernel 0-4MB initially
 * 2. Set global variables of PT for each task's video memory, map 0xB8 to its VRAM
 * 3. Set (0xB9+pid)*4kB in the 0-4MB page to be present
 * @return      0 for success, -1 for fail
 * @effect      PDE, PTE will be changed. TLB will be flushed 
 */
int task_init_video_memory() {

    int i = 0; // loop counter
    int j = 0; // loop counter

    // Set global variables of PT for each terminal's video memory
    // Set (0xB9 + pid) * 4kB in the 0-4MB page to be present
    for (i = 0; i < TERMINAL_MAX_COUNT; i++) {

        // Clear the PTE 
        for (j = 0; j < SIZE_K; j++) {
            clear_PTE((PTE_t *) (&user_video_memory_pt[i].entry[j]));
            clear_PTE((PTE_t *) (&kernel_video_memory_pt[i].entry[j]));
        }

        // User video memory page
        // Map the 0xB8 to corresponding page at (0xB9 + pid) * 4kB
        if (-1 == set_PTE((PTE_t *) (&user_video_memory_pt[i].entry[TASK_VIR_VIDEO_MEM_ENTRY]),
                          (TASK_VIR_VIDEO_MEM_ENTRY + i + 1) * SIZE_4K, 1, 1, 1)) {
            return -1;
        }

        // Kernel video memory page
        // Map the 0xB8 to corresponding page at (0xB9 + pid) * 4kB
        if (-1 == set_PTE((PTE_t *) (&kernel_video_memory_pt[i].entry[TASK_VIR_VIDEO_MEM_ENTRY]),
                          (TASK_VIR_VIDEO_MEM_ENTRY + i + 1) * SIZE_4K, 1, 0, 1)) {
            return -1;
        }

        // Update 0-4MB kernel page
        kernel_page_table_0.entry[TASK_VIR_VIDEO_MEM_ENTRY + i +
                                  1] = kernel_video_memory_pt[i].entry[TASK_VIR_VIDEO_MEM_ENTRY];

        // Update map flag array
        user_video_mapped[i] = TASK_VRAM_NOT_MAPPED;

    }

    FLUSH_TLB();
    return 0;
}

/**
 * Set the PDE for 132MB-136MB to be 4kB page, pointing to corresponding PT 
 * The active PT map directly to physical VRAM
 * @param       id: the id of the inactive terminal VRAM buffer, -1 indicates active, -2 indicates close
 * @return      0 for success, -1 for bad id 
 * @effect      PDE will be changed 
 */
int task_set_user_video_map(const int id) {
    if (id < -2 || id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("task_set_user_video_map(): bad id: %d", id);
        return -1;
    }

    // Get the PDE for 132~136MB
    PDE_4kB_t *user_vram_pde = (PDE_4kB_t *) (&kernel_page_directory.entry[TASK_VIR_MEM_ENTRY + 1]);

    // Clear PDE
    if (-1 == clear_PDE_4kB(user_vram_pde)) return -1;

    // Finish if closing the page
    if (id == -2) return 0;

    // Set the fields (for active id (-1), physical VRAM)
    if (id == -1) {
        if (-1 == set_PDE_4kB(user_vram_pde, (uint32_t) (&user_page_table_0), 1, 1, 1)) return -1;
    } else {
        if (-1 == set_PDE_4kB(user_vram_pde, (uint32_t) (&user_video_memory_pt[id]), 1, 1, 1)) return -1;
    }

    FLUSH_TLB();
    return 0;
}

/**
 * Copy the whole physical video memory to dest_id terminal buffer 
 * @param       dest_id: the destination terminal id
 * @return      0 for success, -1 for bad id 
 * @effect      the original buffer will be covered
 * @note        the paging setting must turn on all buffer, i.e. use kernel_page_table_0
 */
int terminal_copy_from_physical(const int dest_id) {
    // Error checking
    if (dest_id < 0 || dest_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_copy_from_physical(): bad dest_id: %d", dest_id);
        return -1;
    }

    int i; // loop counter 
    // Current copying byte pointer: source (physical VRAM)
    uint8_t *src = (uint8_t *) (TASK_VIR_VIDEO_MEM_ENTRY * SIZE_4K);
    // Current copying byte pointer: buffer (dest_id terminal buffer)
    uint8_t *dest = (uint8_t *) ((TASK_VIR_VIDEO_MEM_ENTRY + dest_id + 1) * SIZE_4K);
    // Copy every byte 
    for (i = 0; i < SIZE_4K; i++) {
        *dest = *src;
        dest++;
        src++;
    }

    return 0;
}

/**
 * Copy the whole src_id terminal buffer to physical video memory
 * @param       src_id: the destination terminal id
 * @return      0 for success, -1 for bad id 
 * @effect      the screen will be changed 
 * @note        the paging setting must turn on all buffer, i.e. use kernel_page_table_0
 */
int terminal_copy_to_physical(const int src_id) {
    // Error checking
    if (src_id < 0 || src_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_copy_to_physical(): bad src_id: %d", src_id);
        return -1;
    }

    int i; // loop counter 
    // Current copying byte pointer: destination (physical VRAM)
    uint8_t *dest = (uint8_t *) (TASK_VIR_VIDEO_MEM_ENTRY * SIZE_4K);
    // Current copying byte pointer: buffer (dest_id terminal buffer)
    uint8_t *src = (uint8_t *) ((TASK_VIR_VIDEO_MEM_ENTRY + src_id + 1) * SIZE_4K);
    // Copy every byte 
    for (i = 0; i < SIZE_4K; i++) {
        *dest = *src;
        src++;
        dest++;
    }

    return 0;
}

/**************** PDE/PTE operations ************/

/**
 * Clear a 4MB PDE by setting all the flags to 0 and address to 0 
 * Then set page_size flag to 1
 * @return      0 for success, -1 for fail
 * @effect      *entry will be changed 
 */
int clear_PDE_4MB(PDE_4MB_t *entry) {
    if (entry == NULL) {
        DEBUG_ERR("clear_PDE_4MB(): bad input!");
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
int clear_PDE_4kB(PDE_4kB_t *entry) {
    if (entry == NULL) {
        DEBUG_ERR("clear_PDE_4kB(): bad input!");
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
int clear_PTE(PTE_t *entry) {
    if (entry == NULL) {
        DEBUG_ERR("clear_PTE(): bad input!");
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
 * Helper function for setting 4kB PDE 
 * only care about base address and can_write, user, and present flag
 * @param       pde: the pde to be set 
 * @param       pt: the address of the page table that this pde pointing to 
 * @param       can_write: can_write flag 
 * @param       user: user_or_super flag 
 * @param       present: present flag 
 * @return      0 for success, -1 for failure 
 */
int set_PDE_4kB(PDE_4kB_t *pde, uint32_t pt, uint8_t can_write, uint8_t user, uint8_t present) {
    // Error checking
    if (pde == NULL || pt == NULL) {
        DEBUG_ERR("set_PDE_4kB(): bad pde ot pt!");
        return -1;
    }
    if (pt & PAGE_4KB_ALIGN_TEST) {
        DEBUG_ERR("set_PDE_4kB(): pt not align! pt:%x", pt);
        return -1;
    }
    if (!(is_valid_flag(can_write) && is_valid_flag(user) && is_valid_flag(present))) {
        DEBUG_ERR("set_PDE_4kB(): bad flag(s)");
        return -1;
    }

    // Set the fields 
    pde->base_address = pt >> 12; // 12: the offset of 4kB address 
    pde->can_write = can_write;
    pde->user_or_super = user;
    pde->present = present;

    return 0;
}

/**
 * Helper function for setting PTE
 * only care about base address and can_write, user, and present flag
 * @param       pte: the pte to be set 
 * @param       page: the address of the 4kB page that this pde pointing to 
 * @param       can_write: can_write flag 
 * @param       user: user_or_super flag 
 * @param       present: present flag 
 * @return      0 for success, -1 for failure 
 */
int set_PTE(PTE_t *pte, uint32_t page, uint8_t can_write, uint8_t user, uint8_t present) {
    // Error checking
    if (pte == NULL || page == NULL) {
        DEBUG_ERR("set_PTE(): bad pde ot pt!");
        return -1;
    }
    if (page & PAGE_4KB_ALIGN_TEST) {
        DEBUG_ERR("set_PTE(): page not align! page:%x", page);
        return -1;
    }
    if (!(is_valid_flag(can_write) && is_valid_flag(user) && is_valid_flag(present))) {
        DEBUG_ERR("set_PTE(): bad flag(s)");
        return -1;
    }

    // Set the fields 
    pte->base_address = page >> 12; // 12: the offset of 4kB address 
    pte->can_write = can_write;
    pte->user_or_super = user;
    pte->present = present;

    return 0;
}

/**
 * Helper function for checking whether flag is 0 or 1 
 * @param       flag: the flag to be checked 
 * @return      1 for yes, 0 for no 
 */
int is_valid_flag(uint8_t flag) {
    return (flag == 0 || flag == 1);
}
