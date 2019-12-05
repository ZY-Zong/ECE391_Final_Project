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

#define gr_readb(off)		(((volatile unsigned char *)VGA_GM)[(off)])
#define gr_readw(off)		(*(volatile unsigned short*)((VGA_GM)+(off)))
#define gr_readl(off)		(*(volatile unsigned long*)((VGA_GM)+(off)))
#define gr_writeb(v,off)	(VGA_GM[(off)] = (v))
#define gr_writew(v,off)	(*(unsigned short*)((VGA_GM)+(off)) = (v))
#define gr_writel(v,off)	(*(unsigned long*)((VGA_GM)+(off)) = (v))


vga_info_t vga_info;            /* current video parameters */
static const vga_info_t CI_G1024_768_16M = {1024, 768, 1 << 24, 1024 * 3, 3};

static int curr_mode = TEXT;
static int curr_page = -1;
static unsigned int curr_color = 0;


static void set_color_emulation(void);

/**
 * Set VGA mode
 * @param mode   Only support G1280x1024x16M (28)
 * @return 0 for success, other values for failure
 */
int vga_setmode(int mode) {

    if (mode != G1024x768x16M) {
        DEBUG_ERR("vag_set_mode(): only support entering G1028x1024x16M mode.");
        return -1;
    }

    mode &= 0xFFF;

    unsigned int interrupt_flags;
    cli_and_save(interrupt_flags);
    {
        curr_mode = mode;

        /* disable video */
        vga_screenoff();
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
            vga_setpage(0);
        }
        vga_screenon();
    }
    restore_flags(interrupt_flags);

    return 0;
}

/**
 * Clear screen
 * @return 0 for success, other values for failure
 */
int vga_clear(void) {

    vga_screenoff();
    {
        int i;
        int pages = (vga_info.ydim * vga_info.xbytes + 65535) >> 16;

        for (i = 0; i < pages; ++i) {
            vga_setpage(i);
            memset(VGA_GM, 0, 65536);
        }

    }
    vga_screenon();

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
void vga_setpage(int page) {
    if (page == curr_page) return;
    cirrus_setpage_64k(page);
    curr_page = page;
}

/**
 * Turn off screen for faster VGA memory access
 */
void vga_screenoff() {
    outb(0x01, SEQ_I);
    outb(inb(SEQ_D) | 0x20, SEQ_D);
}

/**
 * Turn on screen
 */
void vga_screenon() {
    outb(0x01, SEQ_I);
    outb(inb(SEQ_D) & 0xDF, SEQ_D);
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

/**
 * Set color to draw
 * @param rgb    RRGGBB
 */
void vga_setcolor(unsigned rgb) {
    curr_color = rgb;
}

/**
 * Draw a pixel using color set by vga_setcolor()
 * @param x    X coordinate of pixel
 * @param y    Y coordinate of pixel
 * @return 0 for success, other values for failure
 */
int vga_drawpixel(int x, int y) {

    unsigned long offset = y * vga_info.xbytes + x * 3;
    int c = curr_color;

    vga_setpage(offset >> 16);

    switch (offset & 0xffff) {
        case 0xfffe:
            gr_writew(c, 0xfffe);

            vga_setpage((offset >> 16) + 1);
            gr_writeb(c >> 16, 0);
            break;
        case 0xffff:
            gr_writeb(c, 0xffff);

            vga_setpage((offset >> 16) + 1);
            gr_writew(c >> 8, 0);
            break;
        default:
            offset &= 0xffff;
            gr_writew(c, offset);
            gr_writeb(c >> 16, offset + 2);
            break;
    }

    return 0;
}
