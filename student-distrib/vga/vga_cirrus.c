//
// Created by liuzikai on 12/3/19.
// This file is adaptation of cirrus.c from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
// Code here is the instantiation for CIRRUS 5436 (QEMU), 1024×768, 16M (24-bit) color, 60Hz. Much code is eliminated.
//

#include "vga_cirrus.h"

#include "../lib.h"

#include "vga.h"
#include "vga_regs.h"
#include "vga_timming.h"
#include "vga_accel.h"

/* Enable support for > 85 MHz dot clocks on the 5434. */
#define SUPPORT_5434_PALETTE_CLOCK_DOUBLING
/* Use the special clocking mode for all dot clocks at 256 colors, not */
/* just those > 85 MHz, for debugging. */
/* #define ALWAYS_USE_5434_PALETTE_CLOCK_DOUBLING */

#define CIRRUSREG_GR(i) (VGA_TOTAL_REGS + i - VGA_GRAPHICS_COUNT)
#define CIRRUSREG_SR(i) (VGA_TOTAL_REGS + 5 + i - VGA_SEQUENCER_COUNT)
#define CIRRUSREG_CR(i) (VGA_TOTAL_REGS + 5 + 27 + i - VGA_CRTC_COUNT)
#define CIRRUSREG_DAC (VGA_TOTAL_REGS + 5 + 27 + 15)
#define CIRRUS_TOTAL_REGS (VGA_TOTAL_REGS + 5 + 27 + 15 + 1)

/* Indices into mode register array. */
#define CIRRUS_GRAPHICSOFFSET1    CIRRUSREG_GR(0x09)
#define CIRRUS_GRAPHICSOFFSET2    CIRRUSREG_GR(0x0A)
#define CIRRUS_GRB        CIRRUSREG_GR(0x0B)
#define CIRRUS_SR7        CIRRUSREG_SR(0x07)
#define CIRRUS_VCLK3NUMERATOR    CIRRUSREG_SR(0x0E)
#define CIRRUS_DRAMCONTROL    CIRRUSREG_SR(0x0F)
#define CIRRUS_PERFTUNING    CIRRUSREG_SR(0x16)
#define CIRRUS_SR17        CIRRUSREG_SR(0x17)
#define CIRRUS_VCLK3DENOMINATOR CIRRUSREG_SR(0x1E)
#define CIRRUS_MCLKREGISTER    CIRRUSREG_SR(0x1F)
#define CIRRUS_CR19        CIRRUSREG_CR(0x19)
#define CIRRUS_CR1A        CIRRUSREG_CR(0x1A)
#define CIRRUS_CR1B        CIRRUSREG_CR(0x1B)
#define CIRRUS_CR1D        CIRRUSREG_CR(0x1D)
#define CIRRUS_HIDDENDAC    CIRRUSREG_DAC

enum {
    CLGD5420 = 0, CLGD7548, CLGD5420B, CLGD5422, CLGD5422C, CLGD5424, CLGD5426,
    CLGD5428, CLGD5429, CLGD5430, CLGD5434, CLGD5436
};

int cirrus_memory;
static int cirrus_chiptype;
static int cirrus_chiprev;
//static int cirrus_pci_linear = 0;
static unsigned char actualMCLK, programmedMCLK;
static int DRAMbandwidth, DRAMbandwidthLimit;


static void cirrus_initializemode(unsigned char *moderegs,
                                  ModeTiming *modetiming, ModeInfo *modeinfo);

static void __svgalib_createModeInfoStructureForSvgalibMode(ModeInfo *modeinfo, vga_info_t* infotable);

static int cirrus_saveregs(unsigned char regs[]);

static CardSpecs cardspecs;

#define NU_FIXED_CLOCKS 21

/* 12.588 clock (0x33, 0x3B) replaced with 12.599 (0x2C, 0x33). */

static int cirrus_fixed_clocks[NU_FIXED_CLOCKS] =
        {
                12599, 18000, 19600,
                25227, 28325, 31500, 36025, 37747, 39992, 41164,
                45076, 49867, 64983, 72163, 75000, 80013, 85226, 89998,
                95019, 100226, 108035
        };

static unsigned char fixed_clock_numerator[NU_FIXED_CLOCKS] =
        {
                0x2C, 0x27, 0x28,
                0x4A, 0x5B, 0x42, 0x4E, 0x3A, 0x51, 0x45,
                0x55, 0x65, 0x76, 0x7E, 0x6E, 0x5F, 0x7D, 0x58,
                0x49, 0x46, 0x53
        };

static unsigned char fixed_clock_denominator[NU_FIXED_CLOCKS] =
        {
                0x33, 0x3e, 0x3a,
                0x2B, 0x2F, 0x1F, 0x3E, 0x17, 0x3A, 0x30,
                0x36, 0x3A, 0x34, 0x32, 0x2A, 0x22, 0x2A, 0x1C,
                0x16, 0x14, 0x16
        };

enum {
    LINEAR_QUERY_BASE, LINEAR_QUERY_GRANULARITY, LINEAR_QUERY_RANGE,
    LINEAR_ENABLE, LINEAR_DISABLE
};

/* Unlock chipset-specific registers */
static void cirrus_unlock(void) {
    int vgaIOBase, temp;

    /* Are we Mono or Color? */
    vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

    outb(0x06, SEQ_I);
    outb(0x12, SEQ_D);        /* unlock cirrus special */

    /* Put the Vert. Retrace End Reg in temp */

    outb(0x11, vgaIOBase + 4);
    temp = inb(vgaIOBase + 5);

    /* Put it back with PR bit set to 0 */
    /* This unprotects the 0-7 CRTC regs so */
    /* they can be modified, i.e. we can set */
    /* the timing. */

    outb(temp & 0x7F, vgaIOBase + 5);
}

/* Relock chipset-specific registers */
/* (currently not used) */
static void cirrus_lock(void) {
    outb(0x06, SEQ_I);
    outb(0x0F, SEQ_D);        /* relock cirrus special */
}

static int cirrus_init();

/* Bank switching function -- set 64K page number */
void cirrus_setpage_64k(int page) {
    /* Cirrus banking register has been set to 16K granularity */
    outw((page << 10) + 0x09, GRA_I);
}

void cirrus_setpage_4k(int page) {
    /* default 4K granularity */
    outw((page << 12) + 0x09, GRA_I);
}


/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */
void cirrus_setdisplaystart(int address) {
    outw(0x0d + ((address >> 2) & 0x00ff) * 256, CRT_IC);    /* sa2-sa9 */
    outw(0x0c + ((address >> 2) & 0xff00), CRT_IC);    /* sa10-sa17 */
    inb(0x3da);            /* set ATC to addressing mode */
    outb(0x13 + 0x20, ATT_IW);    /* select ATC reg 0x13 */
    /* Cirrus specific bits 0,1 and 18,19,20: */
    outb((inb(ATT_R) & 0xf0) | (address & 3), ATT_IW);
    /* write sa0-1 to bits 0-1; other cards use bits 1-2 */
    outb(0x1b, CRT_IC);
    outb((inb(CRT_DC) & 0xf2)
         | ((address & 0x40000) >> 18)    /* sa18: write to bit 0 */
         | ((address & 0x80000) >> 17)    /* sa19: write to bit 2 */
         | ((address & 0x100000) >> 17), CRT_DC);    /* sa20: write to bit 3 */
    outb(0x1d, CRT_IC);
    if (cirrus_memory > 2048) {
        outb((inb(CRT_DC) & 0x7f) | ((address & 0x200000) >> 14), CRT_DC);    /* sa21: write to bit 7 */
    }
}


/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */
void cirrus_setlogicalwidth(int width) {
    outw(0x13 + (width >> 3) * 256, CRT_IC);    /* lw3-lw11 */
    outb(0x1b, CRT_IC);
    outb((inb(CRT_DC) & 0xef) | ((width & 0x800) >> 7), CRT_DC);
    /* write lw12 to bit 4 of Sequencer reg. 0x1b */
}

void cirrus_setlinear(int addr) {
    int val;
    outb(0x07, SEQ_I);
    val = inb(SEQ_D);
    outb((val & 0x0f) | (addr << 4), SEQ_D);
}

int cirrus_test_and_init() {

    int oldlockreg;
    int lockreg;

    outb(0x06, SEQ_I);
    oldlockreg = inb(SEQ_D);

    cirrus_unlock();

    /* If it's a Cirrus at all, we should be */
    /* able to read back the lock register */

    outb(0x06, SEQ_I);
    lockreg = inb(SEQ_D);

    /* Ok, if it's not 0x12, we're not a Cirrus542X. */
    if (lockreg != 0x12) {
        outb(0x06, SEQ_I);
        outb(oldlockreg, SEQ_D);
        return 0;
    }
    /* The above check seems to be weak, so we also check the chip ID. */

    outb(0x27, __svgalib_CRT_I);
    switch (inb(__svgalib_CRT_D) >> 2) {
        case 0x2E:            /* 5446 */
            break;
        default:
            outb(0x06, SEQ_I);
            outb(oldlockreg, SEQ_D);
            DEBUG_ERR("Error Cirrus Chip");
            return 1;
    }

    if (cirrus_init())
        return 0;        /* failure */

    return 1;
}


static int cirrus_init() {
    unsigned char v;
    cirrus_unlock();

    unsigned char partstatus;
    outb(0x27, __svgalib_CRT_I);
    cirrus_chiptype = inb(__svgalib_CRT_D) >> 2;
    cirrus_chiprev = 0;
    partstatus = __svgalib_inCR(0x25);

    /* Treat 5446 as 5436. */
    cirrus_chiptype = CLGD5436;

    /* Now determine the amount of memory. */
    outb(0x0a, SEQ_I);    /* read memory register */
    /* This depends on the BIOS having set a scratch register. */
    v = inb(SEQ_D);
    cirrus_memory = 256 << ((v >> 3) & 3);

    /* Determine memory the correct way for the 543x, and
     * for the 542x if the amount seems incorrect. */
    unsigned char SRF;
    cirrus_memory = 512;
    outb(0x0f, SEQ_I);
    SRF = inb(SEQ_D);
    if (SRF & 0x10) {
        /* 32-bit DRAM bus. */
        cirrus_memory *= 2;
    }
    if ((SRF & 0x18) == 0x18) {
        /* 64-bit DRAM data bus width; assume 2MB. */
        /* Also indicates 2MB memory on the 5430. */
        cirrus_memory *= 2;
    }
    if (cirrus_chiptype != CLGD5430 && (SRF & 0x80)) {
        /* If DRAM bank switching is enabled, there */
        /* must be twice as much memory installed. */
        /* (4MB on the 5434) */
        cirrus_memory *= 2;
    }


    actualMCLK = __svgalib_inSR(0x1F) & 0x3F;


    programmedMCLK = actualMCLK;

    DRAMbandwidth = 14318 * (int) programmedMCLK / 16;

    /**
     * cirrus_memory = 4096
     */

    if (cirrus_memory >= 512) {
        /* At least 16-bit DRAM bus. */
        DRAMbandwidth *= 2;
    }
    if (cirrus_memory >= 2048) {
        /* 64-bit DRAM bus. */
        DRAMbandwidth *= 2;
    }
    /*
     * Calculate highest acceptable DRAM bandwidth to be taken up
     * by screen refresh. Satisfies
     *     total bandwidth >= refresh bandwidth * 1.1
     */
    DRAMbandwidthLimit = (DRAMbandwidth * 10) / 11;


/* begin: Initialize card specs. */
    cardspecs.videoMemory = cirrus_memory;
    /*
     * First determine clock limits for the chip (DAC), then
     * adjust them according to the available DRAM bandwidth.
     * For 32-bit DRAM cards the 16bpp clock limit is initially
     * set very high, but they are cut down by the DRAM bandwidth
     * check.
     */
    cardspecs.maxPixelClock4bpp = 75000;    /* 5420 */
    cardspecs.maxPixelClock8bpp = 45000;    /* 5420 */
    cardspecs.maxPixelClock16bpp = 0;    /* 5420 */
    cardspecs.maxPixelClock24bpp = 0;
    cardspecs.maxPixelClock32bpp = 0;

    /* 5422/24/26/28 have VCLK spec of 80 MHz. */
    cardspecs.maxPixelClock4bpp = 80000;
    cardspecs.maxPixelClock8bpp = 80000;
    if (cirrus_chiptype >= CLGD5426)
        /* DRAM bandwidth will be limiting factor. */
        cardspecs.maxPixelClock16bpp = 80000;
    else
        /* Clock / 2 16bpp requires 32-bit DRAM bus. */
    if (cirrus_memory >= 1024)
        cardspecs.maxPixelClock16bpp = 80000 / 2;
    /* Clock / 3 24bpp requires 32-bit DRAM bus. */
    if (cirrus_memory >= 1024)
        cardspecs.maxPixelClock24bpp = 80000 / 3;


    /* 5429, 5430, 5434 have VCLK spec of 86 MHz. */
    cardspecs.maxPixelClock4bpp = 86000;
    cardspecs.maxPixelClock8bpp = 86000;
    cardspecs.maxPixelClock16bpp = 86000;
    if (cirrus_memory >= 1024)
        cardspecs.maxPixelClock24bpp = 86000 / 3;


#ifdef SUPPORT_5434_PALETTE_CLOCK_DOUBLING
    cardspecs.maxPixelClock8bpp = 135300;
#endif
    if (cirrus_memory >= 2048)
        /* 32bpp requires 64-bit DRAM bus. */
        cardspecs.maxPixelClock32bpp = 86000;

    cardspecs.maxPixelClock8bpp = min(cardspecs.maxPixelClock8bpp,
                                      DRAMbandwidthLimit);
    cardspecs.maxPixelClock16bpp = min(cardspecs.maxPixelClock16bpp,
                                       DRAMbandwidthLimit / 2);
    cardspecs.maxPixelClock24bpp = min(cardspecs.maxPixelClock24bpp,
                                       DRAMbandwidthLimit / 3);
    cardspecs.maxPixelClock32bpp = min(cardspecs.maxPixelClock32bpp,
                                       DRAMbandwidthLimit / 4);
    cardspecs.flags = INTERLACE_DIVIDE_VERT | GREATER_1024_DIVIDE_VERT;
    /* Initialize clocks (only fixed set for now). */
    cardspecs.nClocks = NU_FIXED_CLOCKS;
    cardspecs.clocks = cirrus_fixed_clocks;
//    cardspecs.mapClock = cirrus_map_clock;
//    cardspecs.mapHorizontalCrtc = cirrus_map_horizontal_crtc;
    cardspecs.maxHorizontalCrtc = 2040;
    /* Disable 16-color SVGA modes (don't work correctly). */
    cardspecs.maxPixelClock4bpp = 0;
/* end: Initialize card specs. */

    __svgalib_mmio_size = 32768;
    __svgalib_mmio_base = 0xb8000;
    if (__svgalib_mmio_size)
        MMIO_POINTER = __svgalib_mmio_base;

    /**
     * {videoMemory = 4096, maxPixelClock4bpp = 0, maxPixelClock8bpp = 135300, maxPixelClock16bpp = 73216,
     *  maxPixelClock24bpp = 28666, maxPixelClock32bpp = 36608, flags = 6,
     *  nClocks = 21,	clocks = 0x40fa00, maxHorizontalCrtc = 2040, mapClock = 0, matchProgrammableClock = 0,
     *  mapHorizontalCrtc = 0}
     */
    return 0;
}

static void writehicolordac(unsigned char c) {
    outb(0, 0x3c6);
    outb(0xff, 0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    outb(c, 0x3c6);
    inb(0x3c8);
}

/* Set chipset-specific registers */
static void cirrus_setregs(const unsigned char regs[]) {

    cirrus_unlock();        /* May be locked again by other programs (eg. X) */

    /* Write extended CRTC registers. */
    __svgalib_outCR(0x19, regs[CIRRUSREG_CR(0x19)]);
    __svgalib_outCR(0x1A, regs[CIRRUSREG_CR(0x1A)]);
    __svgalib_outCR(0x1B, regs[CIRRUSREG_CR(0x1B)]);

    __svgalib_outCR(0x1D, regs[CIRRUSREG_CR(0x1D)]);

    /* Write extended graphics registers. */
    __svgalib_outGR(0x09, regs[CIRRUSREG_GR(0x09)]);
    __svgalib_outGR(0x0A, regs[CIRRUSREG_GR(0x0A)]);
    __svgalib_outGR(0x0B, regs[CIRRUSREG_GR(0x0B)]);

    /* Write Truecolor DAC register. */
    writehicolordac(regs[CIRRUS_HIDDENDAC]);

    /* Write extended sequencer registers. */

    /* Be careful to put the sequencer clocking mode in a safe state. */
    __svgalib_outSR(0x07, (regs[CIRRUS_SR7] & ~0x0F) | 0x01);

    __svgalib_outSR(0x0E, regs[CIRRUS_VCLK3NUMERATOR]);
    __svgalib_outSR(0x1E, regs[CIRRUS_VCLK3DENOMINATOR]);
    __svgalib_outSR(0x07, regs[CIRRUS_SR7]);
    __svgalib_outSR(0x0F, regs[CIRRUS_DRAMCONTROL]);

    __svgalib_outSR(0x16, regs[CIRRUS_PERFTUNING]);
    __svgalib_outSR(0x17, regs[CIRRUS_SR17]);
    __svgalib_outSR(0x1F, regs[CIRRUS_MCLKREGISTER]);
}

int cirrus_setmode(vga_info_t* infotable) {
    unsigned char moderegs[CIRRUS_TOTAL_REGS];
    ModeTiming modetiming;
    ModeInfo modeinfo;

    __svgalib_createModeInfoStructureForSvgalibMode(&modeinfo, infotable);


    if (__svgalib_getmodetiming(&modetiming, &modeinfo, &cardspecs)) {
        DEBUG_ERR("cirrus_setmode(): failed to get timing");
        return 1;
    }

    cirrus_initializemode(moderegs, &modetiming, &modeinfo);

    __svgalib_setregs(moderegs);       /* Set standard regs. */
    cirrus_setregs(moderegs);    /* Set extended regs. */

    __svgalib_InitializeAcceleratorInterface(&modeinfo);

    cirrus_accel_init(modeinfo.bitsPerPixel,
                               modeinfo.lineWidth / modeinfo.bytesPerPixel);

    return 0;
}

static void cirrus_initializemode(unsigned char *moderegs,
                                  ModeTiming *modetiming, ModeInfo *modeinfo) {

    /* Get current values. */
    cirrus_saveregs(moderegs);

    /* Set up the standard VGA registers for a generic SVGA. */
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    /* Set up the extended register values, including modifications */
    /* of standard VGA registers. */

/* Graphics */
    moderegs[CIRRUS_GRAPHICSOFFSET1] = 0;    /* Reset banks. */
    moderegs[CIRRUS_GRAPHICSOFFSET2] = 0;
    moderegs[CIRRUS_GRB] = 0;    /* 0x01 enables dual banking. */
    if (cirrus_memory > 1024)
        /* Enable 16K granularity. */
        moderegs[CIRRUS_GRB] |= 0x20;
    moderegs[VGA_SR2] = 0xFF;    /* Plane mask */

/* CRTC */
    if (modetiming->VTotal >= 1024 && !(modetiming->flags & INTERLACED))
        /*
         * Double the vertical timing. Used for 1280x1024 NI.
         * The CrtcVTimings have already been adjusted
         * by __svgalib_getmodetiming() because of the GREATER_1024_DIVIDE_VERT
         * flag.
         */
        moderegs[VGA_CR17] |= 0x04;
    moderegs[CIRRUS_CR1B] = 0x22;
    if (cirrus_chiptype >= CLGD5434)
        /* Clear display start bit 19. */
        SETBITS(moderegs[CIRRUS_CR1D], 0x80, 0);
    /* CRTC timing overflows. */
    moderegs[CIRRUS_CR1A] = 0;
    SETBITSFROMVALUE(moderegs[CIRRUS_CR1A], 0xC0,
                     modetiming->CrtcVSyncStart + 1, 0x300);
    SETBITSFROMVALUE(moderegs[CIRRUS_CR1A], 0x30,
                     modetiming->CrtcHSyncEnd, (0xC0 << 3));
    moderegs[CIRRUS_CR19] = 0;    /* Interlaced end. */
    if (modetiming->flags & INTERLACED) {
        moderegs[CIRRUS_CR19] =
                ((modetiming->CrtcHTotal / 8) - 5) / 2;
        moderegs[CIRRUS_CR1A] |= 0x01;
    }

/* Scanline offset */
    if (modeinfo->bytesPerPixel == 4) {
        /* At 32bpp the chip does an extra multiplication by two. */
        if (cirrus_chiptype >= CLGD5436) {
            /* Do these chipsets multiply by 4? */
            moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 5;
            SETBITSFROMVALUE(moderegs[CIRRUS_CR1B], 0x10,
                             modeinfo->lineWidth, 0x2000);
        } else {
            moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 4;
            SETBITSFROMVALUE(moderegs[CIRRUS_CR1B], 0x10,
                             modeinfo->lineWidth, 0x1000);
        }
    } else if (modeinfo->bitsPerPixel == 4)
        /* 16 color mode (planar). */
        moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 1;
    else {
        moderegs[VGA_SCANLINEOFFSET] = modeinfo->lineWidth >> 3;
        SETBITSFROMVALUE(moderegs[CIRRUS_CR1B], 0x10,
                         modeinfo->lineWidth, 0x800);
    }

/* Clocking */
    moderegs[VGA_MISCOUTPUT] |= 0x0C;    /* Use VCLK3. */
    moderegs[CIRRUS_VCLK3NUMERATOR] =
            fixed_clock_numerator[modetiming->selectedClockNo];
    moderegs[CIRRUS_VCLK3DENOMINATOR] =
            fixed_clock_denominator[modetiming->selectedClockNo];

/* DAC register and Sequencer Mode */
    {
        unsigned char DAC, SR7;
        DAC = 0x00;
        SR7 = 0x00;
        if (modeinfo->bytesPerPixel > 0)
            SR7 = 0x01;        /* Packed-pixel mode. */
        if (modeinfo->bytesPerPixel == 2) {
            int rgbmode;
            rgbmode = 0;    /* 5-5-5 RGB. */
            if (modeinfo->colorBits == 16)
                rgbmode = 1;    /* Add one for 5-6-5 RGB. */
            if (cirrus_chiptype >= CLGD5426) {
                /* Pixel clock (double edge) mode. */
                DAC = 0xD0 + rgbmode;
                SR7 = 0x07;
            } else {
                /* Single-edge (double VCLK). */
                DAC = 0xF0 + rgbmode;
                SR7 = 0x03;
            }
        }
        if (modeinfo->bytesPerPixel >= 3) {
            /* Set 8-8-8 RGB mode. */
            DAC = 0xE5;
            SR7 = 0x05;
            if (modeinfo->bytesPerPixel == 4)
                SR7 = 0x09;
        }
#ifdef SUPPORT_5434_PALETTE_CLOCK_DOUBLING
        if (modeinfo->bytesPerPixel == 1 && (modetiming->flags & HADJUSTED)) {
            /* Palette clock doubling mode on 5434 8bpp. */
            DAC = 0x4A;
            SR7 = 0x07;
        }
#endif
        moderegs[CIRRUS_HIDDENDAC] = DAC;
        moderegs[CIRRUS_SR7] = SR7;
    }

/* DRAM control and CRT FIFO */
    if (cirrus_chiptype >= CLGD5422)
        /* Enable large CRT FIFO. */
        moderegs[CIRRUS_DRAMCONTROL] |= 0x20;
    if (cirrus_memory == 2048 && cirrus_chiptype <= CLGD5429)
        /* Enable DRAM Bank Select. */
        moderegs[CIRRUS_DRAMCONTROL] |= 0x80;
    if (cirrus_chiptype >= CLGD5424) {
        /* CRT FIFO threshold setting. */
        unsigned char threshold;
        threshold = 8;
/*	if (cirrus_chiptype >= CLGD5434)
	    threshold = 1;*/
        /* XXX Needs more elaborate setting. */
        SETBITS(moderegs[CIRRUS_PERFTUNING], 0x0F, threshold);
    }

    if (programmedMCLK != actualMCLK
        && modeinfo->bytesPerPixel > 0)
        /* Program higher MCLK for packed-pixel modes. */
        moderegs[CIRRUS_MCLKREGISTER] = programmedMCLK;
}

/*
 * This is a temporary function that allocates and fills in a ModeInfo
 * structure based on a svgalib mode number.
 */
/**
 *
 * @param mode
 * @return
 */
static void __svgalib_createModeInfoStructureForSvgalibMode(ModeInfo *modeinfo, vga_info_t* infotable) {

    modeinfo->width = infotable->xdim;
    modeinfo->height = infotable->ydim;
    modeinfo->bytesPerPixel = infotable->bytesperpixel;
    switch (infotable->colors) {
        case 16:
            modeinfo->colorBits = 4;
            break;
        case 256:
            modeinfo->colorBits = 8;
            break;
        case 32768:
            modeinfo->colorBits = 15;
            modeinfo->blueOffset = 0;
            modeinfo->greenOffset = 5;
            modeinfo->redOffset = 10;
            modeinfo->blueWeight = 5;
            modeinfo->greenWeight = 5;
            modeinfo->redWeight = 5;
            break;
        case 65536:
            modeinfo->colorBits = 16;
            modeinfo->blueOffset = 0;
            modeinfo->greenOffset = 5;
            modeinfo->redOffset = 11;
            modeinfo->blueWeight = 5;
            modeinfo->greenWeight = 6;
            modeinfo->redWeight = 5;
            break;
        case 256 * 65536:
            modeinfo->colorBits = 24;
            modeinfo->blueOffset = 0;
            modeinfo->greenOffset = 8;
            modeinfo->redOffset = 16;
            modeinfo->blueWeight = 8;
            modeinfo->greenWeight = 8;
            modeinfo->redWeight = 8;
            break;
    }
    modeinfo->bitsPerPixel = modeinfo->bytesPerPixel * 8;
    if (infotable->colors == 16)
        modeinfo->bitsPerPixel = 4;
    modeinfo->lineWidth = infotable->xbytes;
}


/* Read and save chipset-specific registers */
static int cirrus_saveregs(unsigned char regs[]) {
    cirrus_unlock();        /* May be locked again by other programs (e.g. X) */


    /* Save extended CRTC registers. */
    regs[CIRRUSREG_CR(0x19)] = __svgalib_inCR(0x19);
    regs[CIRRUSREG_CR(0x1A)] = __svgalib_inCR(0x1A);
    regs[CIRRUSREG_CR(0x1B)] = __svgalib_inCR(0x1B);

    regs[CIRRUSREG_CR(0x1D)] = __svgalib_inCR(0x1D);

    /* Save extended graphics registers. */
    regs[CIRRUSREG_GR(0x09)] = __svgalib_inGR(0x09);
    regs[CIRRUSREG_GR(0x0A)] = __svgalib_inGR(0x0A);
    regs[CIRRUSREG_GR(0x0B)] = __svgalib_inGR(0x0B);

    /* Save extended sequencer registers. */
    regs[CIRRUS_SR7] = __svgalib_inSR(0x07);
    regs[CIRRUS_VCLK3NUMERATOR] = __svgalib_inSR(0x0E);
    regs[CIRRUS_DRAMCONTROL] = __svgalib_inSR(0x0F);

    regs[CIRRUS_PERFTUNING] = __svgalib_inSR(0x16);

    regs[CIRRUS_SR17] = __svgalib_inSR(0x17);
    regs[CIRRUS_VCLK3DENOMINATOR] = __svgalib_inSR(0x1E);

    regs[CIRRUS_MCLKREGISTER] = __svgalib_inSR(0x1F);

    /* Save Hicolor DAC register. */
    outb(0, 0x3c6);
    outb(0xff, 0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    inb(0x3c6);
    regs[CIRRUSREG_DAC] = inb(0x3c6);

    return CIRRUS_TOTAL_REGS - VGA_TOTAL_REGS;
}
