//
// Created by liuzikai on 12/7/19.
//

#include "../lib.h"

#include "vga_regs.h"
#include "vga_accel.h"
#include "vga.h"

#define port_outb(port, value) outb((value), (port))
#define port_outw(port, value) outw((value), (port))
#define port_outl(port, value) outl((value), (port))

/* BitBLT modes. */

#define FORWARDS        0x00
#define BACKWARDS        0x01
#define SYSTEMDEST        0x02
#define SYSTEMSRC        0x04
#define TRANSPARENCYCOMPARE    0x08
#define PIXELWIDTH16        0x10
#define PIXELWIDTH32        0x30    /* 543x only. */
#define PATTERNCOPY        0x40
#define COLOREXPAND        0x80

/* Macros for normal I/O BitBLT register access. */

#define SETSRCADDR(addr) \
    port_outw(GRA_I, (((addr) & 0x000000FF) << 8) | 0x2C); \
    port_outw(GRA_I, (((addr) & 0x0000FF00)) | 0x2D); \
    port_outw(GRA_I, (((addr) & 0x003F0000) >> 8) | 0x2E);

#define SETDESTADDR(addr) \
    port_outw(GRA_I, (((addr) & 0x000000FF) << 8) | 0x28); \
    port_outw(GRA_I, (((addr) & 0x0000FF00)) | 0x29); \
    port_outw(GRA_I, (((addr) & 0x003F0000) >> 8) | 0x2A);

/* Pitch: the 5426 goes up to 4095, the 5434 can do 8191. */

#define SETDESTPITCH(pitch) \
    port_outw(GRA_I, (((pitch) & 0x000000FF) << 8) | 0x24); \
    port_outw(GRA_I, (((pitch) & 0x00001F00)) | 0x25);

#define SETSRCPITCH(pitch) \
    port_outw(GRA_I, (((pitch) & 0x000000FF) << 8) | 0x26); \
    port_outw(GRA_I, (((pitch) & 0x00001F00)) | 0x27);

/* Width: the 5426 goes up to 2048, the 5434 can do 8192. */

#define SETWIDTH(width) \
    port_outw(GRA_I, ((((width) - 1) & 0x000000FF) << 8) | 0x20); \
    port_outw(GRA_I, ((((width) - 1) & 0x00001F00)) | 0x21);

/* Height: the 5426 goes up to 1024, the 5434 can do 2048. */
/* It appears many 5434's only go up to 1024. */

#define SETHEIGHT(height) \
    port_outw(GRA_I, ((((height) - 1) & 0x000000FF) << 8) | 0x22); \
    port_outw(GRA_I, (((height) - 1) & 0x00000700) | 0x23);

#define SETBLTMODE(m) \
    port_outw(GRA_I, ((m) << 8) | 0x30);

#define SETBLTWRITEMASK(m) \
    port_outw(GRA_I, ((m) << 8) | 0x2F);

#define SETTRANSPARENCYCOLOR(c) \
    port_outw(GRA_I, ((c) << 8) | 0x34);

#define SETTRANSPARENCYCOLOR16(c) \
    port_outw(GRA_I, ((c) << 8) | 0x34); \
    port_outw(GRA_I, (c & 0xFF00) | 0x35);

#define SETTRANSPARENCYCOLORMASK16(m) \
    port_outw(GRA_I, ((m) << 8) | 0x38); \
    port_outw(GRA_I, ((m) & 0xFF00) | 0x39);

#define SETROP(rop) \
    port_outw(GRA_I, ((rop) << 8) | 0x32);

#define SETFOREGROUNDCOLOR(c) \
    port_outw(GRA_I, 0x01 + ((c) << 8));

#define SETBACKGROUNDCOLOR(c) \
    port_outw(GRA_I, 0x00 + ((c) << 8));

#define SETFOREGROUNDCOLOR16(c) \
    port_outw(GRA_I, 0x01 + ((c) << 8)); \
    port_outw(GRA_I, 0x11 + ((c) & 0xFF00));

#define SETBACKGROUNDCOLOR16(c) \
    port_outw(GRA_I, 0x00 + ((c) << 8)); \
    port_outw(GRA_I, 0x10 + ((c) & 0xFF00)); \

#define SETFOREGROUNDCOLOR32(c) \
    port_outw(GRA_I, 0x01 + ((c) << 8)); \
    port_outw(GRA_I, 0x11 + ((c) & 0xFF00)); \
    port_outw(GRA_I, 0x13 + (((c) & 0xFf0000) >> 8)); \
    port_outw(GRA_I, 0x15 + (((unsigned int)(c) & 0xFF000000) >> 16));

#define SETBACKGROUNDCOLOR32(c) \
    port_outw(GRA_I, 0x00 + ((c) << 8)); \
    port_outw(GRA_I, 0x10 + ((c) & 0xFF00)); \
    port_outw(GRA_I, 0x12 + (((c) & 0xFF0000) >> 8)); \
    port_outw(GRA_I, 0x14 + (((unsigned int)(c) & 0xFF000000) >> 16));

#define STARTBLT() { \
    unsigned char tmp; \
    port_outb(GRA_I, 0x31); \
    tmp = inb(GRA_D); \
    port_outb(GRA_D, tmp | 0x02); \
    }

#define BLTBUSY(s) { \
    port_outb(GRA_I, 0x31); \
    s = inb(GRA_D) & 1; \
    }

#define WAITUNTILFINISHED() \
    for (;;) { \
        int busy; \
        BLTBUSY(busy); \
        if (!busy) \
            break; \
    }


/* Macros for memory-mapped I/O BitBLT register access. */

/* MMIO addresses (offset from 0xb8000). */

#define MMIOBACKGROUNDCOLOR    0x00
#define MMIOFOREGROUNDCOLOR    0x04
#define MMIOWIDTH        0x08
#define MMIOHEIGHT        0x0A
#define MMIODESTPITCH        0x0C
#define MMIOSRCPITCH        0x0E
#define MMIODESTADDR        0x10
#define MMIOSRCADDR        0x14
#define MMIOBLTWRITEMASK    0x17
#define MMIOBLTMODE        0x18
#define MMIOROP            0x1A
#define MMIOBLTSTATUS        0x40

#define MMIOSETDESTADDR(addr) \
  *(unsigned int *)(MMIO_POINTER + MMIODESTADDR) = addr;

#define MMIOSETSRCADDR(addr) \
  *(unsigned int *)(MMIO_POINTER + MMIOSRCADDR) = addr;

/* Pitch: the 5426 goes up to 4095, the 5434 can do 8191. */

#define MMIOSETDESTPITCH(pitch) \
  *(unsigned short *)(MMIO_POINTER + MMIODESTPITCH) = pitch;

#define MMIOSETSRCPITCH(pitch) \
  *(unsigned short *)(MMIO_POINTER + MMIOSRCPITCH) = pitch;

/* Width: the 5426 goes up to 2048, the 5434 can do 8192. */

#define MMIOSETWIDTH(width) \
  *(unsigned short *)(MMIO_POINTER + MMIOWIDTH) = (width) - 1;

/* Height: the 5426 goes up to 1024, the 5434 can do 2048. */

#define MMIOSETHEIGHT(height) \
  *(unsigned short *)(MMIO_POINTER + MMIOHEIGHT) = (height) - 1;

#define MMIOSETBLTMODE(m) \
  *(unsigned char *)(MMIO_POINTER + MMIOBLTMODE) = m;

#define MMIOSETBLTWRITEMASK(m) \
  *(unsigned char *)(MMIO_POINTER + MMIOBLTWRITEMASK) = m;

#define MMIOSETROP(rop) \
  *(unsigned char *)(MMIO_POINTER + MMIOROP) = rop;

#define MMIOSTARTBLT() \
  *(unsigned char *)(MMIO_POINTER + MMIOBLTSTATUS) |= 0x02;

#define MMIOBLTBUSY(s) \
  s = *(volatile unsigned char *)(MMIO_POINTER + MMIOBLTSTATUS) & 1;

#define MMIOSETBACKGROUNDCOLOR(c) \
  *(unsigned char *)(MMIO_POINTER + MMIOBACKGROUNDCOLOR) = c;

#define MMIOSETFOREGROUNDCOLOR(c) \
  *(unsigned char *)(MMIO_POINTER + MMIOFOREGROUNDCOLOR) = c;

#define MMIOSETBACKGROUNDCOLOR16(c) \
  *(unsigned short *)(MMIO_POINTER + MMIOBACKGROUNDCOLOR) = c;

#define MMIOSETFOREGROUNDCOLOR16(c) \
  *(unsigned short *)(MMIO_POINTER + MMIOFOREGROUNDCOLOR) = c;

#define MMIOSETBACKGROUNDCOLOR32(c) \
  *(unsigned int *)(MMIO_POINTER + MMIOBACKGROUNDCOLOR) = c;

#define MMIOSETFOREGROUNDCOLOR32(c) \
  *(unsigned int *)(MMIO_POINTER + MMIOFOREGROUNDCOLOR) = c;

#define MMIOWAITUNTILFINISHED() \
    for (;;) { \
        int busy; \
        MMIOBLTBUSY(busy); \
        if (!busy) \
            break; \
    }

#define FINISHBACKGROUNDBLITS() \
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND) \
        WAITUNTILFINISHED();

#define MMIOFINISHBACKGROUNDBLITS() \
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND) \
        MMIOWAITUNTILFINISHED();

static int cirrus_pattern_address;    /* Pattern with 1's (8 bytes) */
static int cirrus_bitblt_pixelwidth;
/* Foreground color is not preserved on 5420/2/4/6/8. */

int __svgalib_accel_screenpitch;
int __svgalib_accel_bytesperpixel;
int __svgalib_accel_screenpitchinbytes;
int __svgalib_accel_mode;
int __svgalib_accel_bitmaptransparency;

/* Flags for SetMode (accelerator interface). */
#define BLITS_SYNC            0
#define BLITS_IN_BACKGROUND        0x1

unsigned char *MMIO_POINTER;
unsigned long int __svgalib_mmio_base, __svgalib_mmio_size = 0;

void __svgalib_InitializeAcceleratorInterface(ModeInfo *modeinfo) {
    __svgalib_accel_screenpitch = modeinfo->lineWidth / modeinfo->bytesPerPixel;
    __svgalib_accel_bytesperpixel = modeinfo->bytesPerPixel;
    __svgalib_accel_screenpitchinbytes = modeinfo->lineWidth;
    __svgalib_accel_mode = BLITS_SYNC;
    __svgalib_accel_bitmaptransparency = 0;
}


void cirrus_accel_init(int bpp, int width_in_pixels) {
    /* [Setup accelerator screen pitch] */
    /* [Prepare any required off-screen space] */
    if (bpp == 8)
        cirrus_bitblt_pixelwidth = 0;
    if (bpp == 16)
        cirrus_bitblt_pixelwidth = PIXELWIDTH16;
    if (bpp == 32)
        cirrus_bitblt_pixelwidth = PIXELWIDTH32;

    SETSRCPITCH(__svgalib_accel_screenpitchinbytes);
    SETDESTPITCH(__svgalib_accel_screenpitchinbytes);
    SETROP(0x0D);

    cirrus_pattern_address = cirrus_memory * 1024 - 8;

    cirrus_setpage_64k(cirrus_pattern_address / 65536);
    gr_writel(0xffffffff, cirrus_pattern_address & 0xffff);
    gr_writel(0xffffffff, (cirrus_pattern_address & 0xffff) + 4);
    cirrus_setpage_64k(0);

    /* Enable memory-mapped I/O. */
    __svgalib_outSR(0x17, __svgalib_inSR(0x17) | 0x04);
}

void cirrus_accel_screen_copy(int x1, int y1, int x2, int y2, int width, int height) {
    int srcaddr, destaddr, dir;
    width *= __svgalib_accel_bytesperpixel;
    srcaddr = BLTBYTEADDRESS(x1, y1);
    destaddr = BLTBYTEADDRESS(x2, y2);
    dir = FORWARDS;
    if ((y1 < y2 || (y1 == y2 && x1 < x2))
        && y1 + height > y2) {
        srcaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
        destaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
        dir = BACKWARDS;
    }
    FINISHBACKGROUNDBLITS();
    SETSRCADDR(srcaddr);
    SETDESTADDR(destaddr);
    SETWIDTH(width);
    SETHEIGHT(height);
    SETBLTMODE(dir);
    STARTBLT();
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        WAITUNTILFINISHED();
}

void cirrus_accel_mmio_screen_copy(int x1, int y1, int x2, int y2, int width, int height) {
    int srcaddr, destaddr, dir;
    width *= __svgalib_accel_bytesperpixel;
    srcaddr = BLTBYTEADDRESS(x1, y1);
    destaddr = BLTBYTEADDRESS(x2, y2);
    dir = FORWARDS;
    if ((y1 < y2 || (y1 == y2 && x1 < x2))
        && y1 + height > y2) {
        srcaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
        destaddr += (height - 1) * __svgalib_accel_screenpitchinbytes + width - 1;
        dir = BACKWARDS;
    }
    MMIOFINISHBACKGROUNDBLITS();
    MMIOSETSRCADDR(srcaddr);
    MMIOSETDESTADDR(destaddr);
    MMIOSETWIDTH(width);
    MMIOSETHEIGHT(height);
    MMIOSETBLTMODE(dir);
    MMIOSTARTBLT();
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        MMIOWAITUNTILFINISHED();
}

void cirrus_accel_set_foreground_color(int fg)
{
    MMIOFINISHBACKGROUNDBLITS();
    if (__svgalib_accel_bytesperpixel == 1) {
        MMIOSETFOREGROUNDCOLOR(fg);
        return;
    }
    if (__svgalib_accel_bytesperpixel == 2) {
        MMIOSETFOREGROUNDCOLOR16(fg);
        return;
    }
    MMIOSETFOREGROUNDCOLOR32(fg);
}

void cirrus_accel_set_background_color(int bg)
{
    MMIOFINISHBACKGROUNDBLITS();
    if (__svgalib_accel_bytesperpixel == 1) {
        MMIOSETBACKGROUNDCOLOR(bg);
        return;
    }
    if (__svgalib_accel_bytesperpixel == 2) {
        MMIOSETBACKGROUNDCOLOR16(bg);
        return;
    }
    MMIOSETBACKGROUNDCOLOR32(bg);
}

static unsigned char cirrus_rop_map[] =
        {
                0x0D,			/* ROP_COPY */
                0x6D,			/* ROP_OR */
                0x05,			/* ROP_AND */
                0x59,			/* ROP_XOR */
                0x0B			/* ROP_INVERT */
        };

void cirrus_accel_mmio_set_raster_op(int rop)
{
    MMIOFINISHBACKGROUNDBLITS();
    MMIOSETROP(cirrus_rop_map[rop]);
}

void cirrus_accel_mmio_mono_expand(int srcaddr, int x2, int y2, int width, int height, int fg, int bg) {

    cirrus_accel_set_foreground_color(fg);
    cirrus_accel_set_background_color(bg);
    cirrus_accel_mmio_set_raster_op(ROP_COPY);

    int destaddr;
    destaddr = BLTBYTEADDRESS(x2, y2);
    MMIOFINISHBACKGROUNDBLITS();
    MMIOSETSRCADDR(srcaddr);
    MMIOSETDESTADDR(destaddr);
    MMIOSETWIDTH(width);
    MMIOSETHEIGHT(height);
    MMIOSETBLTMODE(COLOREXPAND | PIXELWIDTH16 | SYSTEMSRC);
    MMIOSTARTBLT();
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        MMIOWAITUNTILFINISHED();
}

// FIXME: comment or select a better one
#define BUF_COPY_MID_ADDR ((unsigned int*) 0x000BF000)

void cirrus_accel_mmio_buf_copy(unsigned int* srcaddr, int x2, int y2, int width, int height) {
    int destaddr;
    width *= __svgalib_accel_bytesperpixel;
    destaddr = BLTBYTEADDRESS(x2, y2);
    MMIOFINISHBACKGROUNDBLITS();
    cirrus_accel_mmio_set_raster_op(ROP_COPY);
    MMIOSETSRCADDR((unsigned int) BUF_COPY_MID_ADDR);
    MMIOSETDESTADDR(destaddr);
    MMIOSETWIDTH(width);
    MMIOSETHEIGHT(height);
    MMIOSETBLTMODE(SYSTEMSRC);
    asm volatile ("              \
        movw $0x3CE, %%dx      \n\
        movb $0x31, %%al       \n\
        outb %%al, %%dx        \n\
        movw $0x3CF, %%dx      \n\
        inb %%dx, %%al         \n\
        orb $0x02, %%al        \n\
        outb %%al, %%dx        \n\
    1:  movl (%%ebx), %%eax    \n\
        movl %%eax, 0xBF000    \n\
        addl $4, %%ebx         \n\
        dec %%ecx              \n\
        jg 1b"                    \
        :                         \
        : "b" (srcaddr), "c" (width * height * __svgalib_accel_bytesperpixel / sizeof(unsigned int))
        : "memory", "cc");
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        MMIOWAITUNTILFINISHED();
}