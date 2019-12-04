//
// Created by liuzikai on 12/3/19.
//

#include "vga.h"
#include "vga_port.h"

#include "../lib.h"
#include "vga_driver.h"
#include "vga_cirrus.h"

// TODO: change all port_out to outb and all port_in to inb

int prev_mode;
int curr_mode = TEXT;

DriverSpecs *__svgalib_driverspecs = NULL;

unsigned char *BANKED_MEM_POINTER = NULL, *LINEAR_MEM_POINTER, *MMIO_POINTER;
unsigned char *B8000_MEM_POINTER = NULL;
unsigned long int __svgalib_banked_mem_base, __svgalib_banked_mem_size;
unsigned long int __svgalib_mmio_base, __svgalib_mmio_size = 0;
unsigned long int __svgalib_linear_mem_base = 0, __svgalib_linear_mem_size = 0;

// TODO: rename this variable
struct info CI;            /* current video parameters */
struct info CI_G1024_768_16M = {1024, 768, 1 << 24, 1024 * 3, 3};

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

    mode &= 0xfff;

    unsigned int interrupt_flags;
    cli_and_save(interrupt_flags);
    {

        prev_mode = curr_mode;
        curr_mode = mode;

        /* disable video */
        vga_screenoff();
        {
            /* shift to color emulation */
            setcoloremulation();

            CI = CI_G1024_768_16M;

            cirrus_setmode(mode, prev_mode);

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

    // TODO: implement this function
    /*int i;
    int pages = (CI.ydim * CI.xbytes + 65535) >> 16;

    for (i = 0; i < pages; ++i) {
        vga_setpage(i);


        *//* clear video memory *//*
        memset(GM, 0, 65536);
    }


    vga_setcolor(15);*/

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
    // TODO: implement current page judgement
    cirrus_setpage_2M(page);
    // cirrus_setpage(page);
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
