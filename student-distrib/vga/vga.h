//
// Created by liuzikai on 12/3/19.
// This file is adaptation of various files from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
//

/**
 * QEMU uses chip set of Cirrus 5446 (treated as CLGD5436)
 * We only support entering mode 1024×768, 32K, 60Hz from text mode, which will be performed at startup
 */

#ifndef _VGA_H
#define _VGA_H

#include "../lib.h"

#include "vga_draw.h"

#define VGA_WIDTH              1024
#define VGA_HEIGHT             768
#define VGA_BYTES_PER_PIXEL    2
#define VGA_BYTES_PER_LINE     (VGA_WIDTH * VGA_BYTES_PER_PIXEL)
#define VGA_SCREEN_BYTES       (VGA_WIDTH * VGA_HEIGHT * VGA_BYTES_PER_PIXEL)

#define VGA_PAGE_SIZE    (0x10000)

#define TEXT 	          0
#define G1024x768x32K     23

#define GM    ((char *) VIDEO)
#define gr_readb(off)        (((volatile unsigned char *)GM)[(off)])
#define gr_readw(off)        (*(volatile unsigned short*)((GM)+(off)))
#define gr_readl(off)        (*(volatile unsigned long*)((GM)+(off)))
#define gr_writeb(v, off)    (GM[(off)] = (v))
#define gr_writew(v, off)    (*(unsigned short*)((GM)+(off)) = (v))
#define gr_writel(v, off)    (*(unsigned long*)((GM)+(off)) = (v))

void vga_init();
int vga_set_mode(int mode);
int vga_clear(void);

void vga_set_page(int page);
void vga_set_start_addr(int address);

void vga_screen_off();
void vga_screen_on();

void vga_screen_copy(int x1, int y1, int x2, int y2, int width, int height);
//void vga_buf_copy(unsigned int* srcaddr, int x2, int y2, int width, int height);

#define DISABLE_TRANSPARENCY_COLOR	0
#define ENABLE_TRANSPARENCY_COLOR	1
#define DISABLE_BITMAP_TRANSPARENCY	2
#define ENABLE_BITMAP_TRANSPARENCY	3

void vga_set_transparent(int mode, int color);

void vga_accel_sync();

#define BLITS_SYNC            0
#define BLITS_IN_BACKGROUND   0x1

void vga_accel_set_mode(int m);

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
