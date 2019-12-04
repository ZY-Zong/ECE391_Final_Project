//
// Created by liuzikai on 12/3/19.
//

#ifndef _VGA_CIRRUS_H
#define _VGA_CIRRUS_H

#include "vga_port.h"

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

/* Cards specifications. */
typedef struct {
    int videoMemory;		/* Video memory in kilobytes. */
    int maxPixelClock4bpp;	/* Maximum pixel clocks in kHz for each depth. */
    int maxPixelClock8bpp;
    int maxPixelClock16bpp;
    int maxPixelClock24bpp;
    int maxPixelClock32bpp;
    int flags;			/* Flags (e.g. programmable clocks). */
    int nClocks;		/* Number of fixed clocks. */
    int *clocks;		/* Pointer to array of fixed clock values. */
    int maxHorizontalCrtc;
    /*
     * The following function maps from a pixel clock and depth to
     * the raw clock frequency required.
     */
    int (*mapClock) (int bpp, int pixelclock);
    /*
     * The following function maps from a requested clock value
     * to the closest clock that the programmable clock device
     * can produce.
     */
    int (*matchProgrammableClock) (int desiredclock);
    /*
     * The following function maps from a pixel clock, depth and
     * horizontal CRTC timing parameter to the horizontal timing
     * that has to be programmed.
     */
    int (*mapHorizontalCrtc) (int bpp, int pixelclock, int htiming);
} CardSpecs;

#define CIRRUSREG_GR(i) (VGA_TOTAL_REGS + i - VGA_GRAPHICS_COUNT)
#define CIRRUSREG_SR(i) (VGA_TOTAL_REGS + 5 + i - VGA_SEQUENCER_COUNT)
#define CIRRUSREG_CR(i) (VGA_TOTAL_REGS + 5 + 27 + i - VGA_CRTC_COUNT)
#define CIRRUSREG_DAC (VGA_TOTAL_REGS + 5 + 27 + 15)
#define CIRRUS_TOTAL_REGS (VGA_TOTAL_REGS + 5 + 27 + 15 + 1)


#endif //_VGA_CIRRUS_H
