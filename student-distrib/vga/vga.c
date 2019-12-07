//
// Created by liuzikai on 12/3/19.
// This file is adaptation of various files from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
// Code here is the instantiation for CIRRUS 5436 (QEMU), 1024×768, 16M (24-bit) color, 60Hz. Much code is eliminated.
//

#include "vga.h"

#include "../lib.h"

#include "vga_port.h"
#include "vga_regs.h"
#include "vga_cirrus.h"
#include "vga_accel.h"

static void __svgalib_delay(void);

#define VGA_CHECK_CURRENT_PAGE    1

vga_info_t vga_info;            /* current video parameters */
static const vga_info_t CI_G1024_768_16M = {1024, 768, 1 << 24, 1024 * 3, 3};

static int curr_page = -1;


static void set_color_emulation(void);

/**
 * Set VGA mode
 * @param mode   Only support G1280x1024x16M (28)
 * @return 0 for success, other values for failure
 */
int vga_set_mode(int mode) {

    if (mode != G1024x768x16M) {
        DEBUG_ERR("vag_set_mode(): only support entering G1028x1024x16M mode.");
        return -1;
    }

    mode &= 0xFFF;

    unsigned int interrupt_flags;
    cli_and_save(interrupt_flags);
    {
        /* disable video */
        vga_screen_off();
        {
            /* shift to color emulation */
            set_color_emulation();

            vga_info = CI_G1024_768_16M;

            cirrus_setmode(mode);

            cirrus_setdisplaystart(0);
            cirrus_setlogicalwidth(1024 * 3);
            cirrus_setlinear(0);

            /* clear screen (sets current color to 15) */
            vga_clear();
            vga_set_page(0);
        }
        vga_screen_on();
    }
    restore_flags(interrupt_flags);

    return 0;
}

/**
 * Clear screen
 * @return 0 for success, other values for failure
 */
int vga_clear(void) {

    vga_screen_off();
    {
        int i;
        int pages = (vga_info.ydim * vga_info.xbytes + 65535) >> 16;

        for (i = 0; i < pages; ++i) {
            vga_set_page(i);
            memset(GM, 0, 65536);
        }

    }
    vga_screen_on();

    return 0;
}

/**
 * Helper function to enable color emulation
 */
static void set_color_emulation(void) {
    /* shift to color emulation */
    __svgalib_CRT_I = CRT_IC;
    __svgalib_CRT_D = CRT_DC;
    __svgalib_IS1_R = IS1_RC;
    outb(inb(MIS_R) | 0x01, MIS_W);
}

/**
 * Set VGA paging
 * @param page
 */
//void vga_set_page(int page) {
//#if VGA_CHECK_CURRENT_PAGE
//    if (page == curr_page) return;
//#endif
//    cirrus_setpage_64k(page);
//#if VGA_CHECK_CURRENT_PAGE
//    curr_page = page;
//#endif
//}

/**
 * Turn off screen for faster VGA memory access
 */
void vga_screen_off() {
    outb(0x01, SEQ_I);
    outb(inb(SEQ_D) | 0x20, SEQ_D);

    inb(__svgalib_IS1_R);
    __svgalib_delay();
    outb(0x00, ATT_IW);
}

/**
 * Turn on screen
 */
void vga_screen_on() {
    outb(0x01, SEQ_I);
    outb(inb(SEQ_D) & 0xDF, SEQ_D);

    inb(__svgalib_IS1_R);
    __svgalib_delay();
    outb(0x20, ATT_IW);
}

/**
 * Initialize VGA driver
 */
void vga_init() {
    unsigned int interrupt_flags;
    cli_and_save(interrupt_flags);
    {
        cirrus_test_and_init();
    }
}

static void __svgalib_delay() {
    int i;
    for (i = 0; i < 10; i++);
}

void vga_screen_copy(int x1, int y1, int x2, int y2, int width, int height) {
    cirrus_accel_mmio_screen_copy(x1, y1, x2, y2, width, height);
}

void vga_buf_copy(int srcaddr, int x2, int y2, int width, int height) {
    cirrus_accel_mmio_buf_copy(srcaddr, x2, y2, width, height);
}
