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
#define G1024x768x16M     25

// TODO: rename this structure
/* graphics mode information */
struct info {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
};

/* Acceleration interface. */

/* Accel operations. */
#define ACCEL_FILLBOX			1	/* Simple solid fill. */
#define ACCEL_SCREENCOPY		2	/* Simple screen-to-screen BLT. */
#define ACCEL_PUTIMAGE			3	/* Straight image transfer. */
#define ACCEL_DRAWLINE			4	/* General line draw. */
#define ACCEL_SETFGCOLOR		5	/* Set foreground color. */
#define ACCEL_SETBGCOLOR		6	/* Set background color. */
#define ACCEL_SETTRANSPARENCY		7	/* Set transparency mode. */
#define ACCEL_SETRASTEROP		8	/* Set raster-operation. */
#define ACCEL_PUTBITMAP			9	/* Color-expand bitmap. */
#define ACCEL_SCREENCOPYBITMAP		10	/* Color-expand from screen. */
#define ACCEL_DRAWHLINELIST		11	/* Draw horizontal spans. */
#define ACCEL_SETMODE			12	/* Set blit strategy. */
#define ACCEL_SYNC			13	/* Wait for blits to finish. */
#define ACCEL_SETOFFSET			14	/* Set screen offset */
#define ACCEL_SCREENCOPYMONO		15	/* Monochrome screen-to-screen BLT. */
#define ACCEL_POLYLINE			16	/* Draw multiple lines. */
#define ACCEL_POLYHLINE			17	/* Draw multiple horizontal spans. */
#define ACCEL_POLYFILLMODE		18	/* Set polygon mode. */

/* Corresponding bitmask. */
#define ACCELFLAG_FILLBOX		0x1	/* Simple solid fill. */
#define ACCELFLAG_SCREENCOPY		0x2	/* Simple screen-to-screen BLT. */
#define ACCELFLAG_PUTIMAGE		0x4	/* Straight image transfer. */
#define ACCELFLAG_DRAWLINE		0x8	/* General line draw. */
#define ACCELFLAG_SETFGCOLOR		0x10	/* Set foreground color. */
#define ACCELFLAG_SETBGCOLOR		0x20	/* Set background color. */
#define ACCELFLAG_SETTRANSPARENCY	0x40	/* Set transparency mode. */
#define ACCELFLAG_SETRASTEROP		0x80	/* Set raster-operation. */
#define ACCELFLAG_PUTBITMAP		0x100	/* Color-expand bitmap. */
#define ACCELFLAG_SCREENCOPYBITMAP	0x200	/* Color-exand from screen. */
#define ACCELFLAG_DRAWHLINELIST		0x400	/* Draw horizontal spans. */
#define ACCELFLAG_SETMODE		0x800	/* Set blit strategy. */
#define ACCELFLAG_SYNC			0x1000	/* Wait for blits to finish. */
#define ACCELFLAG_SETOFFSET		0x2000	/* Set screen offset */
#define ACCELFLAG_SCREENCOPYMONO	0x4000	/* Monochrome screen-to-screen BLT. */
#define ACCELFLAG_POLYLINE		0x8000	/* Draw multiple lines. */
#define ACCELFLAG_POLYHLINE		0x10000	/* Draw multiple horizontal spans. */
#define ACCELFLAG_POLYFILLMODE		0x20000	/* Set polygon mode. */

/* Mode for SetTransparency. */
#define DISABLE_TRANSPARENCY_COLOR	0
#define ENABLE_TRANSPARENCY_COLOR	1
#define DISABLE_BITMAP_TRANSPARENCY	2
#define ENABLE_BITMAP_TRANSPARENCY	3

/* Flags for SetMode (accelerator interface). */
#define BLITS_SYNC			0
#define BLITS_IN_BACKGROUND		0x1

/* Raster ops. */
#define ROP_COPY			0	/* Straight copy. */
#define ROP_OR				1	/* Source OR destination. */
#define ROP_AND				2	/* Source AND destination. */
#define ROP_XOR				3	/* Source XOR destination. */
#define ROP_INVERT			4	/* Invert destination. */

/* For the poly funcs */
#define ACCEL_START			1
#define ACCEL_END			2

extern unsigned long int __svgalib_banked_mem_base, __svgalib_banked_mem_size;
extern unsigned long int __svgalib_mmio_base, __svgalib_mmio_size;
extern unsigned long int __svgalib_linear_mem_base, __svgalib_linear_mem_size;

extern unsigned char * BANKED_MEM_POINTER, *LINEAR_MEM_POINTER, *MMIO_POINTER;
extern unsigned char * B8000_MEM_POINTER;

#endif //_VGA_H
