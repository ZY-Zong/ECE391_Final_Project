//
// Created by liuzikai on 12/3/19.
//

#include "vga.h"

#include "../lib.h"

#include "vga_port.h"
#include "vga_regs.h"
#include "vga_cirrus.h"

int curr_mode = TEXT;

vga_info_t vga_info;            /* current video parameters */
static const vga_info_t CI_G1024_768_16M = {1024, 768, 1 << 24, 1024 * 3, 3};

static int curr_page = -1;

static void setcoloremulation(void);

/**
 * Transplanted function.
 * @param mode   Only support G1280x1024x16M (28)
 * @return
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
            setcoloremulation();

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

static void setcoloremulation(void) {
    /* shift to color emulation */
    __svgalib_CRT_I = CRT_IC;
    __svgalib_CRT_D = CRT_DC;
    __svgalib_IS1_R = IS1_RC;
    outb(inb(MIS_R) | 0x01, MIS_W);
}

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

void vga_init() {
    unsigned int interrupt_flags;
    cli_and_save(interrupt_flags);
    {
        cirrus_test_and_init();
    }
}
