//
// Created by liuzikai on 12/3/19.
//

#ifndef _VGA_H
#define _VGA_H

extern int __svgalib_CRT_I;		/* current CRT index register address */
extern int __svgalib_CRT_D;		/* current CRT data register address */
extern int __svgalib_IS1_R;		/* current input status register address */

/**
 * QEMU uses chip set of Cirrus 5446 (treated as CLGD5436)
 * We only support entering mode 1024 × 768 16M 60Hz from text mode
 */

void vga_screenoff();
void vga_screenon();

#define TEXT 	          0
// FIXME: 1024x768x16M
#define G1280x1024x16M    28

// TODO: rename this structure
/* graphics mode information */
struct info {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
};

#endif //_VGA_H
