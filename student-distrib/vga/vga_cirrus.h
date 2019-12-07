//
// Created by liuzikai on 12/3/19.
// This file is adaptation of cirrus.h from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
// Code here is the instantiation for CIRRUS 5436 (QEMU), 1024×768, 16M (24-bit) color, 60Hz. Much code is eliminated.
//

#ifndef _VGA_CIRRUS_H
#define _VGA_CIRRUS_H

#include "../lib.h"
#include "vga_port.h"

int cirrus_test_and_init();
int cirrus_setmode(int mode);
void cirrus_setdisplaystart(int address);
void cirrus_setlogicalwidth(int width);
void cirrus_setlinear(int addr);

void cirrus_setpage_64k(int page);
void cirrus_setpage_4k(int page);

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

extern int cirrus_memory;

#endif //_VGA_CIRRUS_H
