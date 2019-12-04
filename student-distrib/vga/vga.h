//
// Created by liuzikai on 12/3/19.
//

#ifndef _VGA_H
#define _VGA_H

#include "../lib.h"

#include "vga_pixel.h"

/**
 * QEMU uses chip set of Cirrus 5446 (treated as CLGD5436)
 * We only support entering mode 1024 × 768 16M 60Hz from text mode
 */
#define TEXT 	          0
#define G1024x768x16M     25

#define VGA_GM    ((char *) VIDEO)

void vga_init();
int vga_setmode(int mode);
void vga_setpage(int page);
int vga_clear(void);
void vga_screenoff();
void vga_screenon();

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
