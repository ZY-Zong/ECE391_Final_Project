//
// Created by liuzikai on 12/3/19.
//

#ifndef _VGA_H
#define _VGA_H

/**
 * QEMU uses chip set of Cirrus 5446 (treated as CLGD5436)
 * We only support entering mode 1024 × 768 16M 60Hz from text mode
 */

#define TEXT 	          0
#define G1024x768x16M     25

int vga_setmode(int mode);
void vga_setpage(int page);
int vga_clear(void);
void vga_screenoff();
void vga_screenon();


// TODO: rename this structure
/* graphics mode information */
struct info {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
};

extern unsigned long int __svgalib_banked_mem_base, __svgalib_banked_mem_size;
extern unsigned long int __svgalib_mmio_base, __svgalib_mmio_size;
extern unsigned long int __svgalib_linear_mem_base, __svgalib_linear_mem_size;

extern unsigned char * BANKED_MEM_POINTER, *LINEAR_MEM_POINTER, *MMIO_POINTER;
extern unsigned char * B8000_MEM_POINTER;

#endif //_VGA_H
