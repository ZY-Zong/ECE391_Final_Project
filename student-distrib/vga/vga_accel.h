//
// Created by liuzikai on 12/7/19.
//

#ifndef _VGA_ACCEL_H
#define _VGA_ACCEL_H

#include "vga_timming.h"

/* Flags: */
/* Every programmable scanline width is supported by the accelerator. */

#define ACCELERATE_ANY_LINEWIDTH	0x1
/* Bitmap (1-bit-per-pixel) operations support transparency (bit = 0). */
#define BITMAP_TRANSPARENCY		0x2
/* For bitmaps (1 bpp) stored in video memory, the most-significant bit */
/* within a byte is the leftmost pixel. */
#define BITMAP_ORDER_MSB_FIRST		0x4

/* Operation flags: see vga.h. */

/*
 * Acceleration primitive description:
 *
 * FillBox      Simple solid fill of rectangle with a single color.
 * ScreenCopy   Screen-to-screen BLT (BitBlt), handles overlapping areas.
 * PutImage     Straight image transfer (PutImage). Advantage over
 *              framebuffer writes is mainly in alignment handling.
 * DrawLine     Draw general line ("zero-pixel wide").
 * SetFGColor   Set foreground color for some operations (FillBox, DrawLine,
 *              PutBitmap).
 * SetBGColor   Set background color for some operations (PutBitmap).
 * SetRasterOp  Set the raster operation for drawing operations that support
 *              raster ops as defined in ropOperations.
 * SetTransparency
 *              Set the transparency mode for some operations (enable/disable,
 *              and the transparency pixel value). Source pixels equal to
 *              the transparency color are not written. Operations supported
 *              are ScreenCopy and PutImage, subject to their flags being set
 *              in the transparencyOperations field.
 * PutBitmap    Color-expand a bit-wise (bit-per-pixel, each byte is 8 pixels)
 *              image to the screen with the foreground and background color.
 *              The lowest order bit of each byte is leftmost on the screen
 *              (contrary to the VGA tradition), irrespective of the bitmap
 *              bit order flag. Each scanline is aligned to a multiple of
 *              32-bits.
 *              If the transparency mode is enabled (irrespective of the
 *              transparency color), then bits that are zero in the bitmap
 *              are not written (the background color is not used).
 * ScreenCopyBitmap
 *              Color-expand bit-wise bitmap stored in video memory
 *              (may also support transparency).
 * DrawHLineList
 *              Draw a set of horizontal line segments from top to bottom
 *              in the foreground color.
 * SetMode      Set the acceleration mode, e.g. let blits go
 *              on in the background (program must not access video memory
 *              when blits can be running).
 * Sync         Wait for any background blits to finish.
 *
 * It is not the intention to have alternative non-accelerated routines
 * available for each possible operation (library functions should
 * take advantage of accelerator functions, rather than the accelerator
 * functions being primitives on their own right). If something like
 * bit-order reversal is required to implement an accelerated primitive,
 * it's still worthwhile if it's still much quicker than similar
 * unaccelerated functionality would be.
 *
 * Strategy for accelerator registers in accelerated functions:
 *      Foreground color, background color, raster operation and transparency
 *      compare setting are preserved, source and destination pitch is always
 *      set to screen pitch (may be temporarily changed and then restored).
 */


/* Macros. */

#define BLTBYTEADDRESS(x, y) \
	(y * __svgalib_accel_screenpitchinbytes + x * __svgalib_accel_bytesperpixel)

#define BLTPIXADDRESS(x, y) \
	(y * __svgalib_accel_screenpitch + x)

#define SIGNALBLOCK \
    {								\
         sigset_t sig2block;					\
	 sigemptyset(&sig2block); 				\
	 sigaddset(&sig2block,SIGINT); 				\
	 sigprocmask(SIG_BLOCK, &sig2block, (sigset_t *)NULL);	\
     }

#define SIGNALUNBLOCK \
    {								\
         sigset_t sig2block;					\
	 sigemptyset(&sig2block); 				\
	 sigaddset(&sig2block,SIGINT); 				\
	 sigprocmask(SIG_UNBLOCK, &sig2block, (sigset_t *)NULL);\
     }

/* Variables defined in accel.c */

extern int __svgalib_accel_screenpitch;
extern int __svgalib_accel_bytesperpixel;
extern int __svgalib_accel_screenpitchinbytes;
extern int __svgalib_accel_mode;
extern int __svgalib_accel_bitmaptransparency;

/*
 * The following function should be called when the mode is set.
 * This is currently done in the setmode driver functions.
 */

void __svgalib_InitializeAcceleratorInterface(ModeInfo * modeinfo);

extern unsigned char *MMIO_POINTER;
extern unsigned long int __svgalib_mmio_base, __svgalib_mmio_size;

void cirrus_accel_init(int bpp, int width_in_pixels);
void cirrus_accel_screen_copy(int x1, int y1, int x2, int y2, int width, int height);
void cirrus_accel_mmio_screen_copy(int x1, int y1, int x2, int y2, int width, int height);


#endif //_VGA_ACCEL_H
