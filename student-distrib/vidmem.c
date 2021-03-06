//
// Created by liuzikai on 11/29/19.
//

#include "vidmem.h"

#include "lib.h"
#include "paging.h"
#include "x86_desc.h"

#include "task/task.h"
#include "terminal.h"
#include "file_system.h"

#define     VIDMEM_PAGE_ENTRY         0xBF

#define     TASK_IMG_START            0x08000000  // 128MB
#define     TASK_IMG_END              0x08400000  // 132MB
#define     VIDMAP_PAGE_ENTRY         33          // 132MB / 4MB
#define     VIDMAP_USER_START_ADDR    (TASK_IMG_END + VIDMEM_PAGE_ENTRY * SIZE_4K)  // 132MB + 0xA0 * 4K

#define     USER_VRAM_PAGE_ENTRY        0x0
#define     VRAM_BUFFER_PD_ENTRY        (KERNEL_PAGE_OFFSET + 2)

#define     TERMINAL_VID_OPENED         777     // funny number
#define     TERMINAL_VID_NOT_OPENED     0

// Video memory
static int terminal_active;
static int terminal_running;
static int terminal_opened[TERMINAL_MAX_COUNT];
static page_table_t user_video_memory_pt[TERMINAL_MAX_COUNT];
static page_table_t kernel_video_memory_pt[TERMINAL_MAX_COUNT];

/* has been modified for svga */
/**
 * Reference for page tables
 * kernel_page_table_0: PDE 0~4MB
 *      the page that can access physical VRAM (modeX)
 *      useful for copying from buffers and write the screen 
 * kernel_page_table_1: PDE VRAM_BUFFER_PD_ENTRY * 4MB ~ +1 
 *      (no need to set for copy since no one will change it)
 *      the page that can access all buffer 
 *      useful for copying from buffers and write the screen
 * kernel_video_memory_pt: PDE 0~4MB
 *      the page for terminal, mapping to its buffer
 * user_page_table_0: deleted 
 * user_video_memory_pt: PDE 132~136MB
 *      the page for user process on terminal, mapping to its buffer
 */


void vidmem_init() {
    int i;
    int j;

    for (i = 0; i < TERMINAL_MAX_COUNT; i++) {
        terminal_opened[i] = TERMINAL_VID_NOT_OPENED;
    }
    terminal_active = NULL_TERMINAL_ID;
    terminal_running = NULL_TERMINAL_ID;

    // Set global variables of PT for each terminal's video memory
    // Set (0xB9 + pid) * 4kB in the 0-4MB page to be present
    for (i = 0; i < TERMINAL_MAX_COUNT; i++) {

        // Clear the PTE
        for (j = 0; j < SIZE_K; j++) {
            clear_PTE((PTE_t *) (&user_video_memory_pt[i].entry[j]));
            clear_PTE((PTE_t *) (&kernel_video_memory_pt[i].entry[j]));
        }

        // User video memory page
        // Map the 0xA0 to corresponding page at (VRAM_BUFFER_PD_ENTRY * ADDRESS_4MB) + (0xB9 + pid) * 4kB
        set_PTE((PTE_t *) (&user_video_memory_pt[i].entry[VIDMEM_PAGE_ENTRY]),
                (VRAM_BUFFER_PD_ENTRY * ADDRESS_4MB) + (USER_VRAM_PAGE_ENTRY + i) * SIZE_4K, 1, 1, 1);

        // Kernel video memory page
        // Map the 0xA0 to corresponding page at (VRAM_BUFFER_PD_ENTRY * ADDRESS_4MB) + (0xB9 + pid) * 4kB
        set_PTE((PTE_t *) (&kernel_video_memory_pt[i].entry[VIDMEM_PAGE_ENTRY]),
                (VRAM_BUFFER_PD_ENTRY * ADDRESS_4MB) + (USER_VRAM_PAGE_ENTRY + i) * SIZE_4K, 1, 0, 1);

        // Upate kernel_page_table_1 for copying 
        kernel_page_table_1.entry[USER_VRAM_PAGE_ENTRY + i] = kernel_video_memory_pt[i].entry[VIDMEM_PAGE_ENTRY];
        
        /* no longer needed for svga */ 
        // Update 0-4MB kernel page
        // kernel_page_table_0.entry[USER_VRAM_PAGE_ENTRY + i] = kernel_video_memory_pt[i].entry[VIDMEM_PAGE_ENTRY];

    }
    // update the PDE for vidmem buffer 
    set_PDE_4kB( (PDE_4kB_t *)(&kernel_page_directory.entry[KERNEL_PAGE_OFFSET + 2]) , 
                (uint32_t) &kernel_page_table_1, 1, 0, 1);

    FLUSH_TLB();
}

/**
 * Open a video memory that is not yet opened
 * Set up the paging mapping for it
 * But not set it to active
 * @param term_id    the terminal to be open , 0 <= ter_id < TERMINAL_MAX_COUNT
 * @return          0 for success, -1 for fail
 * @effect          PDE will be changed
 */
int terminal_vidmem_open(const int term_id, char** char_on_screen) {
    if (term_id < 0 || term_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_vidmem_open(): no such ter_id: %d", term_id);
        return -1;
    }
    if (terminal_opened[term_id] == TERMINAL_VID_OPENED) {
        DEBUG_WARN("terminal_vidmem_open(): already opened ter_id: %d\n", term_id);
    }

    terminal_opened[term_id] = TERMINAL_VID_OPENED;
    *char_on_screen = (char*)( (VRAM_BUFFER_PD_ENTRY * ADDRESS_4MB) + 
                        (USER_VRAM_PAGE_ENTRY + term_id) * SIZE_4K );

    return 0;
}

/**
 * Close a terminal video memory that is opened
 * IMPORTANT: the terminal to close should not be active
 * if want to close a terminal, please call switch first
 * @param term_id    the terminal to be open , 0 <= ter_id < TERMINAL_MAX_COUNT
 * @return          0 for success, -1 for fail
 * @effect          PDE will be changed
 */
int terminal_vidmem_close(const int term_id) {
    if (term_id < 0 || term_id >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_vidmem_close(): no such ter_id: %d", term_id);
        return -1;
    }
    if (term_id == terminal_active) {
        DEBUG_ERR("terminal_vidmem_close(): cannot close active ter_id: %d", term_id);
        return -1;
    }
    if (terminal_opened[term_id] == TERMINAL_VID_NOT_OPENED) {
        DEBUG_WARN("terminal_vidmem_open(): already closed ter_id: %d\n", term_id);
    }

    terminal_opened[term_id] = TERMINAL_VID_NOT_OPENED;

    return 0;
}

/**
 * Set current paging pointing VRAM to buffer for running terminal
 * @param term_id   The terminal to be set
 * @return          0 for success, -1 for fail
 * @effect          PDE will be changed
 */
int terminal_vidmem_set(const int term_id) {

    // Error checking
    if (term_id != NULL_TERMINAL_ID && (term_id < 0 || term_id >= TERMINAL_MAX_COUNT)) {
        DEBUG_ERR("terminal_vidmem_set(): bad ter_id: %d", term_id);
        return -1;
    }
    if (term_id != NULL_TERMINAL_ID && terminal_opened[term_id] == TERMINAL_VID_NOT_OPENED) {
        DEBUG_ERR("terminal_vidmem_set(): unopened ter_id: %d", term_id);
        return -1;
    }

    // Set the PDE for 0-4MB pointing to corresponding PT
    /* no longer needed in svga */
    if (term_id == NULL_TERMINAL_ID ) { // NULL_TERMINAL_ID: draw the screen 
        if (-1 == set_PDE_4kB((PDE_4kB_t *) (&kernel_page_directory.entry[0]),
                              (uint32_t) &kernel_page_table_0, 1, 0, 1))
            return -1;
    } else {
        if (-1 == set_PDE_4kB((PDE_4kB_t *) (&kernel_page_directory.entry[0]),
                              (uint32_t) &kernel_video_memory_pt[term_id], 1, 0, 1))
            return -1;
    }

    terminal_running = term_id;  // for focus switch

    FLUSH_TLB();
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
    if ((int) screen_start < TASK_IMG_START || (int) screen_start >= TASK_IMG_END) {
        DEBUG_ERR("system_vidmap(): screen_start out of range: %x", (int) screen_start);
        return -1;
    }

    // Check whether the current task ter_id correct
    if (running_task()->terminal->terminal_id == NULL_TERMINAL_ID) {
        DEBUG_ERR("system_vidmap(): current task has no terminal");
        return -1;
    }

    running_task()->vidmap_enabled = 1;

    task_set_user_vidmap(running_task()->terminal->terminal_id);

    *screen_start = (uint8_t *) VIDMAP_USER_START_ADDR;

    return 0;
}

 /**
  * Set or close user vidmap between 132-136MB
  * @param term_id    Terminal ID that user vidmap maps to. NULL_TERMINAL_ID to close user vidmap.
  */
void task_set_user_vidmap(int term_id) {

    // Get the PDE for 132~136MB
    PDE_4kB_t *user_vram_pde = (PDE_4kB_t *) (&kernel_page_directory.entry[VIDMAP_PAGE_ENTRY]);

    if (term_id == NULL_TERMINAL_ID) {
        // Close user vidmap
        clear_PDE_4kB(user_vram_pde);
    } else {
        /* no longer needed for svga */
        // if (term_id == terminal_active) {
        //     // Map to focus terminal
        //     set_PDE_4kB(user_vram_pde, (uint32_t) (&user_page_table_0), 1, 1, 1);
        // } else {
            // Map to background terminal
            set_PDE_4kB(user_vram_pde, (uint32_t) (&user_video_memory_pt[term_id]), 1, 1, 1);
        // }
    }

    FLUSH_TLB();
}

