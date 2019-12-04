//
// Created by liuzikai on 12/3/19.
//

#ifndef _VGA_PORT_H
#define _VGA_PORT_H

/* VGA index register ports */
#define CRT_IC  0x3D4		/* CRT Controller Index - color emulation */
#define CRT_IM  0x3B4		/* CRT Controller Index - mono emulation */
#define ATT_IW  0x3C0		/* Attribute Controller Index & Data Write Register */
#define GRA_I   0x3CE		/* Graphics Controller Index */
#define SEQ_I   0x3C4		/* Sequencer Index */
#define PEL_IW  0x3C8		/* PEL Write Index */
#define PEL_IR  0x3C7		/* PEL Read Index */

/* VGA data register ports */
#define CRT_DC  0x3D5		/* CRT Controller Data Register - color emulation */
#define CRT_DM  0x3B5		/* CRT Controller Data Register - mono emulation */
#define ATT_R   0x3C1		/* Attribute Controller Data Read Register */
#define GRA_D   0x3CF		/* Graphics Controller Data Register */
#define SEQ_D   0x3C5		/* Sequencer Data Register */
#define MIS_R   0x3CC		/* Misc Output Read Register */
#define MIS_W   0x3C2		/* Misc Output Write Register */
#define IS1_RC  0x3DA		/* Input Status Register 1 - color emulation */
#define IS1_RM  0x3BA		/* Input Status Register 1 - mono emulation */
#define PEL_D   0x3C9		/* PEL Data Register */
#define PEL_MSK 0x3C6		/* PEL mask register */


/* Register indices into mode state array. */

#define VGA_CRTC_COUNT		24
#define VGA_ATC_COUNT		21
#define VGA_GRAPHICS_COUNT	9
#define VGA_SEQUENCER_COUNT	5

#define VGA_CRTC_OFFSET		0	/* 24 registers */
#define VGA_ATC_OFFSET		24	/* 21 registers */
#define VGA_GRAPHICS_OFFSET	45	/* 9 registers. */
#define VGA_SEQUENCER_OFFSET	54	/* 5 registers. */
#define VGA_MISCOUTPUT		59	/* (single register) */
#define VGA_TOTAL_REGS		60

#endif //_VGA_PORT_H
