//
// Created by liuzikai on 11/29/19.
//

#include "paging.h"

#include "lib.h"

int is_valid_flag(uint8_t flag);

/**************** PDE/PTE Operations ************/

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
