//
// Created by liuzikai on 12/3/19.
// This file is adaptation of timing.c from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
// Code here is the instantiation for CIRRUS 5436 (QEMU), 1024×768, 16M (24-bit) color, 60Hz. Much code is eliminated.
//

#include "vga_cirrus.h"
#include "vga_timming.h"
#include "../lib.h"

/* 1024x768 at 60 Hz, 48.4 kHz hsync */
MonitorModeTiming TIMMING_1024_768_60HZ = {65000, 1024, 1032, 1176, 1344, 768, 771, 777, 806, NHSYNC | NVSYNC, NULL};

/*
 * Clock allowance in 1/1000ths. 10 (1%) corresponds to a 250 kHz
 * deviation at 25 MHz, 1 MHz at 100 MHz
 */
#define CLOCK_ALLOWANCE 10

#define PROGRAMMABLE_CLOCK_MAGIC_NUMBER 0x1234

static int findclock(int clock, CardSpecs *cardspecs) {
    int i;
    /* Find a clock that is close enough. */
    for (i = 0; i < cardspecs->nClocks; i++) {
        int diff;
        diff = cardspecs->clocks[i] - clock;
        if (diff < 0)
            diff = -diff;
        if (diff * 1000 / clock < CLOCK_ALLOWANCE)
            return i;
    }
    /* Try programmable clocks if available. */
    if (cardspecs->flags & CLOCK_PROGRAMMABLE) {
        int diff;
        diff = cardspecs->matchProgrammableClock(clock) - clock;
        if (diff < 0)
            diff = -diff;
        if (diff * 1000 / clock < CLOCK_ALLOWANCE)
            return PROGRAMMABLE_CLOCK_MAGIC_NUMBER;
    }
    /* No close enough clock found. */
    return -1;
}

int __svgalib_getmodetiming(ModeTiming *modetiming, ModeInfo *modeinfo,
                            CardSpecs *cardspecs) {
    int desiredclock;
    MonitorModeTiming *besttiming = NULL;

    /*
     * Check user defined timings first.
     * If there is no match within these, check the standard timings.
     */
    besttiming = &TIMMING_1024_768_60HZ;

    /*
     * Copy the selected timings into the result, which may
     * be adjusted for the chipset.
     */

    modetiming->flags = besttiming->flags;
    modetiming->pixelClock = besttiming->pixelClock;    /* Formal clock. */

    /*
     * We know a close enough clock is available; the following is the
     * exact clock that fits the mode. This is probably different
     * from the best matching clock that will be programmed.
     */
    desiredclock = besttiming->pixelClock;

    /* Fill in the best-matching clock that will be programmed. */
    modetiming->selectedClockNo = findclock(desiredclock, cardspecs);

    modetiming->programmedClock = cardspecs->clocks[modetiming->selectedClockNo];

    modetiming->HDisplay = besttiming->HDisplay;
    modetiming->HSyncStart = besttiming->HSyncStart;
    modetiming->HSyncEnd = besttiming->HSyncEnd;
    modetiming->HTotal = besttiming->HTotal;

    modetiming->CrtcHDisplay = besttiming->HDisplay;
    modetiming->CrtcHSyncStart = besttiming->HSyncStart;
    modetiming->CrtcHSyncEnd = besttiming->HSyncEnd;
    modetiming->CrtcHTotal = besttiming->HTotal;

    modetiming->VDisplay = besttiming->VDisplay;
    modetiming->VSyncStart = besttiming->VSyncStart;
    modetiming->VSyncEnd = besttiming->VSyncEnd;
    modetiming->VTotal = besttiming->VTotal;
    if (modetiming->flags & DOUBLESCAN) {
        modetiming->VDisplay <<= 1;
        modetiming->VSyncStart <<= 1;
        modetiming->VSyncEnd <<= 1;
        modetiming->VTotal <<= 1;
    }
    modetiming->CrtcVDisplay = modetiming->VDisplay;
    modetiming->CrtcVSyncStart = modetiming->VSyncStart;
    modetiming->CrtcVSyncEnd = modetiming->VSyncEnd;
    modetiming->CrtcVTotal = modetiming->VTotal;
    if (((modetiming->flags & INTERLACED)
         && (cardspecs->flags & INTERLACE_DIVIDE_VERT))
        || (modetiming->VTotal >= 1024
            && (cardspecs->flags & GREATER_1024_DIVIDE_VERT))) {
        /*
         * Card requires vertical CRTC timing to be halved for
         * interlaced modes, or for all modes with vertical
         * timing >= 1024.
         */
        modetiming->CrtcVDisplay /= 2;
        modetiming->CrtcVSyncStart /= 2;
        modetiming->CrtcVSyncEnd /= 2;
        modetiming->CrtcVTotal /= 2;
        modetiming->flags |= VADJUSTED;
    }
    return 0;            /* Succesful. */
}
