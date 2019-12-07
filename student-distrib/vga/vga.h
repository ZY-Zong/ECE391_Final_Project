//
// Created by liuzikai on 12/3/19.
// This file is adaptation of various files from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
// Code here is the instantiation for CIRRUS 5436 (QEMU), 1024×768, 16M (24-bit) color, 60Hz. Much code is eliminated.
//

/**
 * QEMU uses chip set of Cirrus 5446 (treated as CLGD5436)
 * We only support entering mode 1024×768, 16M, 60Hz from text mode
 */

#ifndef _VGA_H
#define _VGA_H

#include "../lib.h"

#include "vga_cirrus.h"
#include "vga_draw.h"

#define TEXT 	          0
#define G1024x768x16M     25

#define GM    ((char *) VIDEO)
//#undef GM
//#define GM gm_buffer
#define gr_readb(off)        (((volatile unsigned char *)GM)[(off)])
#define gr_readw(off)        (*(volatile unsigned short*)((GM)+(off)))
#define gr_readl(off)        (*(volatile unsigned long*)((GM)+(off)))
#define gr_writeb(v, off)    (GM[(off)] = (v))
#define gr_writew(v, off)    (*(unsigned short*)((GM)+(off)) = (v))
#define gr_writel(v, off)    (*(unsigned long*)((GM)+(off)) = (v))

void vga_init();
int vga_set_mode(int mode);
int vga_clear(void);

static void inline vga_set_page(int page) {
    cirrus_setpage_64k(page);
}

void vga_screen_off();
void vga_screen_on();

void vga_screen_copy(int x1, int y1, int x2, int y2, int width, int height);

/* graphics mode information */
typedef struct vga_info_t vga_info_t;
struct vga_info_t {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
};

extern vga_info_t vga_info;

#endif //_VGA_H
