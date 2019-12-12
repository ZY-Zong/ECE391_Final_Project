//
// Created by liuzikai on 12/3/19.
// This file is adaptation of timing.h from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
// Code here is the instantiation for CIRRUS 5436 (QEMU), 1024×768, 16M (24-bit) color, 60Hz. Much code is eliminated.
//

#ifndef _VGA_TIMMING_H
#define _VGA_TIMMING_H

/* Flags in ModeTiming. */
#define PHSYNC		0x1	/* Positive hsync polarity. */
#define NHSYNC		0x2	/* Negative hsync polarity. */
#define PVSYNC		0x4	/* Positive vsync polarity. */
#define NVSYNC		0x8	/* Negative vsync polarity. */
#define INTERLACED	0x10	/* Mode has interlaced timing. */
#define DOUBLESCAN	0x20	/* Mode uses VGA doublescan (see note). */
#define HADJUSTED	0x40	/* Horizontal CRTC timing adjusted. */
#define VADJUSTED	0x80	/* Vertical CRTC timing adjusted. */
#define USEPROGRCLOCK	0x100	/* A programmable clock is used. */

/* This is for the hardware (card)-adjusted mode timing. */
typedef struct {
    int pixelClock;		/* Pixel clock in kHz. */
    int HDisplay;		/* Horizontal Timing. */
    int HSyncStart;
    int HSyncEnd;
    int HTotal;
    int VDisplay;		/* Vertical Timing. */
    int VSyncStart;
    int VSyncEnd;
    int VTotal;
    int flags;
/* The following field are optionally filled in according to card */
/* specific parameters. */
    int programmedClock;	/* Actual clock to be programmed. */
    int selectedClockNo;	/* Index number of fixed clock used. */
    int CrtcHDisplay;		/* Actual programmed horizontal CRTC timing. */
    int CrtcHSyncStart;
    int CrtcHSyncEnd;
    int CrtcHTotal;
    int CrtcVDisplay;		/* Actual programmed vertical CRTC timing. */
    int CrtcVSyncStart;
    int CrtcVSyncEnd;
    int CrtcVTotal;
} ModeTiming;

/* Mode info. */
typedef struct {
/* Basic properties. */
    short width;		/* Width of the screen in pixels. */
    short height;		/* Height of the screen in pixels. */
    char bytesPerPixel;		/* Number of bytes per pixel. */
    char bitsPerPixel;		/* Number of bits per pixel. */
    char colorBits;		/* Number of significant bits in pixel. */
    char __padding1;
/* Truecolor pixel specification. */
    char redWeight;		/* Number of significant red bits. */
    char greenWeight;		/* Number of significant green bits. */
    char blueWeight;		/* Number of significant blue bits. */
    char __padding2;
    char redOffset;		/* Offset in bits of red value into pixel. */
    char blueOffset;		/* Offset of green value. */
    char greenOffset;		/* Offset of blue value. */
    char __padding3;
    unsigned redMask;		/* Pixel mask of read value. */
    unsigned blueMask;		/* Pixel mask of green value. */
    unsigned greenMask;		/* Pixel mask of blue value. */
/* Structural properties of the mode. */
    int lineWidth;		/* Offset in bytes between scanlines. */
    short realWidth;		/* Real on-screen resolution. */
    short realHeight;		/* Real on-screen resolution. */
    int flags;
} ModeInfo;

/* This is the type of a basic (monitor-oriented) mode timing. */
typedef struct _MMT_S MonitorModeTiming;
struct _MMT_S {
    int pixelClock;		/* Pixel clock in kHz. */
    int HDisplay;		/* Horizontal Timing. */
    int HSyncStart;
    int HSyncEnd;
    int HTotal;
    int VDisplay;		/* Vertical Timing. */
    int VSyncStart;
    int VSyncEnd;
    int VTotal;
    int flags;
    MonitorModeTiming *next;
};

/* Card flags. */
/* The card has programmable clocks (matchProgrammableClock is valid). */
#define CLOCK_PROGRAMMABLE		0x1
/* For interlaced modes, the vertical timing must be divided by two. */
#define INTERLACE_DIVIDE_VERT		0x2
/* For modes with vertical timing greater or equal to 1024, vertical */
/* timing must be divided by two. */
#define GREATER_1024_DIVIDE_VERT	0x4
/* The DAC doesn't support 64K colors (5-6-5) at 16bpp, just 5-5-5. */
#define NO_RGB16_565			0x8

int __svgalib_getmodetiming(ModeTiming * modetiming, ModeInfo * modeinfo,
                            CardSpecs * cardspecs);


#endif //_VGA_TIMMING_H
