//
// Created by liuzikai on 12/3/19.
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
