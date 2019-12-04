//
// Created by liuzikai on 12/3/19.
//

#include "vga.h"
#include "vga_timming.h"
#include "vga_accel.h"

int __svgalib_accel_screenpitch;
int __svgalib_accel_bytesperpixel;
int __svgalib_accel_screenpitchinbytes;
int __svgalib_accel_mode;
int __svgalib_accel_bitmaptransparency;

void __svgalib_InitializeAcceleratorInterface(ModeInfo * modeinfo)
{
    __svgalib_accel_screenpitch = modeinfo->lineWidth / modeinfo->bytesPerPixel;
    __svgalib_accel_bytesperpixel = modeinfo->bytesPerPixel;
    __svgalib_accel_screenpitchinbytes = modeinfo->lineWidth;
    __svgalib_accel_mode = BLITS_SYNC;
    __svgalib_accel_bitmaptransparency = 0;
}