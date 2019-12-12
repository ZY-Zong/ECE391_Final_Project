//
// Created by liuzikai on 12/3/19.
// This file is adaptation of vgaregs.c from SVGAlib 1.4.3. See SVGALIB_LICENSE for license.
// Code here is the instantiation for CIRRUS 5436 (QEMU), 1024×768, 16M (24-bit) color, 60Hz. Much code is eliminated.
//

#include "vga.h"
#include "vga_regs.h"
#include "vga_cirrus.h"
#include "vga_timming.h"

#include "../lib.h"

int __svgalib_CRT_I = CRT_IC;        /* current CRT index register address */
int __svgalib_CRT_D = CRT_DC;        /* current CRT data register address */
int __svgalib_IS1_R = IS1_RC;        /* current input status register address */

/*
 * Setup VGA registers for SVGA mode timing. Adapted from XFree86,
 * vga256/vga/vgaHW.c vgaHWInit().
 *
 * Note that VGA registers are set up in a way that is common for
 * SVGA modes. This is not particularly useful for standard VGA
 * modes, since VGA does not have a clean packed-pixel mode.
 */

void __svgalib_setup_VGA_registers(unsigned char *moderegs, ModeTiming * modetiming,
                                   ModeInfo * modeinfo)
{
    int i;
/* Sync Polarities */
    if ((modetiming->flags & (PHSYNC | NHSYNC)) &&
        (modetiming->flags & (PVSYNC | NVSYNC))) {
        /*
         * If both horizontal and vertical polarity are specified,
         * set them as specified.
         */
        moderegs[VGA_MISCOUTPUT] = 0x23;
        if (modetiming->flags & NHSYNC)
            moderegs[VGA_MISCOUTPUT] |= 0x40;
        if (modetiming->flags & NVSYNC)
            moderegs[VGA_MISCOUTPUT] |= 0x80;
    } else {
        /*
         * Otherwise, calculate the polarities according to
         * monitor standards.
         */
        if (modetiming->VDisplay < 400)
            moderegs[VGA_MISCOUTPUT] = 0xA3;
        else if (modetiming->VDisplay < 480)
            moderegs[VGA_MISCOUTPUT] = 0x63;
        else if (modetiming->VDisplay < 768)
            moderegs[VGA_MISCOUTPUT] = 0xE3;
        else
            moderegs[VGA_MISCOUTPUT] = 0x23;
    }

/* Sequencer */
    moderegs[VGA_SR0] = 0x00;
    if (modeinfo->bitsPerPixel == 4)
        moderegs[VGA_SR0] = 0x02;
    moderegs[VGA_SR1] = 0x01;
    moderegs[VGA_SR2] = 0x0F;	/* Bitplanes. */
    moderegs[VGA_SR3] = 0x00;
    moderegs[VGA_SR4] = 0x0E;
    if (modeinfo->bitsPerPixel == 4)
        moderegs[VGA_SR4] = 0x06;

/* CRTC Timing */
    moderegs[VGA_CR0] = (modetiming->CrtcHTotal / 8) - 5;
    moderegs[VGA_CR1] = (modetiming->CrtcHDisplay / 8) - 1;
    moderegs[VGA_CR2] = (modetiming->CrtcHSyncStart / 8) - 1;
    moderegs[VGA_CR3] = ((modetiming->CrtcHSyncEnd / 8) & 0x1F) | 0x80;
    moderegs[VGA_CR4] = (modetiming->CrtcHSyncStart / 8);
    moderegs[VGA_CR5] = (((modetiming->CrtcHSyncEnd / 8) & 0x20) << 2)
                        | ((modetiming->CrtcHSyncEnd / 8) & 0x1F);
    moderegs[VGA_CR6] = (modetiming->CrtcVTotal - 2) & 0xFF;
    moderegs[VGA_CR7] = (((modetiming->CrtcVTotal - 2) & 0x100) >> 8)
                        | (((modetiming->CrtcVDisplay - 1) & 0x100) >> 7)
                        | ((modetiming->CrtcVSyncStart & 0x100) >> 6)
                        | (((modetiming->CrtcVSyncStart) & 0x100) >> 5)
                        | 0x10
                        | (((modetiming->CrtcVTotal - 2) & 0x200) >> 4)
                        | (((modetiming->CrtcVDisplay - 1) & 0x200) >> 3)
                        | ((modetiming->CrtcVSyncStart & 0x200) >> 2);
    moderegs[VGA_CR8] = 0x00;
    moderegs[VGA_CR9] = ((modetiming->CrtcVSyncStart & 0x200) >> 4) | 0x40;
    if (modetiming->flags & DOUBLESCAN)
        moderegs[VGA_CR9] |= 0x80;
    moderegs[VGA_CRA] = 0x00;
    moderegs[VGA_CRB] = 0x00;
    moderegs[VGA_CRC] = 0x00;
    moderegs[VGA_CRD] = 0x00;
    moderegs[VGA_CRE] = 0x00;
    moderegs[VGA_CRF] = 0x00;
    moderegs[VGA_CR10] = modetiming->CrtcVSyncStart & 0xFF;
    moderegs[VGA_CR11] = (modetiming->CrtcVSyncEnd & 0x0F) | 0x20;
    moderegs[VGA_CR12] = (modetiming->CrtcVDisplay - 1) & 0xFF;
    moderegs[VGA_CR13] = modeinfo->lineWidth >> 4;	/* Just a guess. */
    moderegs[VGA_CR14] = 0x00;
    moderegs[VGA_CR15] = modetiming->CrtcVSyncStart & 0xFF;
    moderegs[VGA_CR16] = (modetiming->CrtcVSyncStart + 1) & 0xFF;
    moderegs[VGA_CR17] = 0xC3;
    if (modeinfo->bitsPerPixel == 4)
        moderegs[VGA_CR17] = 0xE3;
    moderegs[VGA_CR18] = 0xFF;

/* Graphics Controller */
    moderegs[VGA_GR0] = 0x00;
    moderegs[VGA_GR1] = 0x00;
    moderegs[VGA_GR2] = 0x00;
    moderegs[VGA_GR3] = 0x00;
    moderegs[VGA_GR4] = 0x00;
    moderegs[VGA_GR5] = 0x40;
    if (modeinfo->bitsPerPixel == 4)
        moderegs[VGA_GR5] = 0x02;
    moderegs[VGA_GR6] = 0x05;
    moderegs[VGA_GR7] = 0x0F;
    moderegs[VGA_GR8] = 0xFF;

/* Attribute Controller */
    for (i = 0; i < 16; i++)
        moderegs[VGA_AR0 + i] = i;
    moderegs[VGA_AR10] = 0x41;
    if (modeinfo->bitsPerPixel == 4)
        moderegs[VGA_AR10] = 0x01;	/* was 0x81 */
    /* Attribute register 0x11 is the overscan color. */
    moderegs[VGA_AR12] = 0x0F;
    moderegs[VGA_AR13] = 0x00;
    moderegs[VGA_AR14] = 0x00;
}

/*
 * These are simple functions to write a value to a VGA Graphics
 * Controller, CRTC Controller or Sequencer register.
 * The idea is that drivers call these as a function call, making the
 * code smaller and inserting small delays between I/O accesses when
 * doing mode switches.
 */

void __svgalib_outGR(int index, unsigned char val)
{
    int v;
    v = ((int) val << 8) + index;
    outw(v, GRA_I);
}

void __svgalib_outbGR(int index, unsigned char val)
{
    outb(index, GRA_I);
    outb(val, GRA_D);
}

void __svgalib_outCR(int index, unsigned char val)
{
    int v;
    v = ((int) val << 8) + index;
    outw(v, CRT_IC);
}

void __svgalib_outbCR(int index, unsigned char val)
{
    outb(index, CRT_IC);
    outb(val, __svgalib_CRT_D);
}

void __svgalib_outSR(int index, unsigned char val)
{
    int v;
    v = ((int) val << 8) + index;
    outw(v, SEQ_I);
}

void __svgalib_outbSR(int index, unsigned char val)
{
    outb(index, SEQ_I);
    outb(val, SEQ_D);
}

unsigned char __svgalib_inGR(int index)
{
    outb(index, GRA_I);
    return inb(GRA_D);
}

unsigned char __svgalib_inCR(int index)
{
    outb(index, CRT_IC);
    return inb(CRT_DC);
}

unsigned char __svgalib_inSR(int index)
{
    outb(index, SEQ_I);
    return inb(SEQ_D);
}

int __svgalib_inmisc(void)
{
    return inb(MIS_R);
}

void __svgalib_outmisc(int i)
{
    outb(MIS_W,i);
}

void __svgalib_delay(void)
{
    int i;
    for (i = 0; i < 10; i++);
}

int __svgalib_setregs(const unsigned char *regs)
{
    int i;

    /* update misc output register */
    __svgalib_outmisc(regs[MIS]);

    /* synchronous reset on */
    __svgalib_outseq(0x00,0x01);

    /* write sequencer registers */
    __svgalib_outseq(0x01,regs[SEQ + 1] | 0x20);
    outb(1, SEQ_I);
    outb(regs[SEQ + 1] | 0x20, SEQ_D);
    for (i = 2; i < SEQ_C; i++) {
        __svgalib_outseq(i,regs[SEQ + i]);
    }

    /* synchronous reset off */
    __svgalib_outseq(0x00,0x03);


    /* deprotect CRT registers 0-7 */
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);


    /* write CRT registers */
    for (i = 0; i < CRT_C; i++) {
        __svgalib_outcrtc(i,regs[CRT + i]);
    }

    /* write graphics controller registers */
    for (i = 0; i < GRA_C; i++) {
        outb(i, GRA_I);
        outb(regs[GRA + i], GRA_D);
    }

    /* write attribute controller registers */
    for (i = 0; i < ATT_C; i++) {
        inb(__svgalib_IS1_R);		/* reset flip-flop */
        __svgalib_delay();
        outb(i, ATT_IW);
        __svgalib_delay();
        outb(regs[ATT + i], ATT_IW);
        __svgalib_delay();
    }

    return 0;
}
