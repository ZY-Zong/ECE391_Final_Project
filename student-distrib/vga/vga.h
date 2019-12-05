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

#define TEXT 	          0
#define G1024x768x16M     25

#define VGA_GM    ((char *) VIDEO)

void vga_init();
int vga_setmode(int mode);
void vga_setpage(int page);
int vga_clear(void);
void vga_screenoff();
void vga_screenon();

void vga_setcolor(unsigned int rgb);
int vga_drawpixel(int x, int y);

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
