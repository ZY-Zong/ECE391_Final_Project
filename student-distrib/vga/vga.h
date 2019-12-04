//
// Created by liuzikai on 12/3/19.
//

#ifndef _VGA_H
#define _VGA_H

#include "vga_pixel.h"

/**
 * QEMU uses chip set of Cirrus 5446 (treated as CLGD5436)
 * We only support entering mode 1024 × 768 16M 60Hz from text mode
 */

#define TEXT 	          0
#define G1024x768x16M     25

void vga_init();
int vga_setmode(int mode);
void vga_setpage(int page);
int vga_clear(void);
void vga_screenoff();
void vga_screenon();

/* graphics mode information */
struct vga_info {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
};

extern struct vga_info CI;

#endif //_VGA_H
