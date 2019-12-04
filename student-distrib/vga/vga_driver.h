//
// Created by liuzikai on 12/3/19.
//

#ifndef _VGA_DRIVER_H
#define _VGA_DRIVER_H

#define MAX_REGS 5000 /* VESA needs a lot of storage space */

typedef struct {
    void (*savepalette)(unsigned char *red, unsigned char *green, unsigned char *blue);
    void (*restorepalette)(const unsigned char *red,
                           const unsigned char *green, const unsigned char *blue);
    int  (*setpalette)(int index, int red, int green, int blue);
    void (*getpalette)(int index, int *red, int *green, int *blue);
    void (*savefont)(void);
    void (*restorefont)(void);
    int (*screenoff)(void);
    int (*screenon)(void);
    void (*waitretrace)(void);
} Emulation;

typedef struct {
/* Basic functions. */
    int (*saveregs) (unsigned char regs[]);
    void (*setregs) (const unsigned char regs[], int mode);
    void (*unlock) (void);
    void (*lock) (void);
    int (*test) (void);
    int (*init) (int force, int par1, int par2);
    void (*__svgalib_setpage) (int page);
    void (*__svgalib_setrdpage) (int page);
    void (*__svgalib_setwrpage) (int page);
    int (*setmode) (int mode, int prv_mode);
    int (*modeavailable) (int mode);
    void (*setdisplaystart) (int address);
    void (*setlogicalwidth) (int width);
    void (*getmodeinfo) (int mode, vga_modeinfo * modeinfo);
/* Obsolete blit functions. */
    void (*bitblt) (int srcaddr, int destaddr, int w, int h, int pitch);
    void (*imageblt) (void *srcaddr, int destaddr, int w, int h, int pitch);
    void (*fillblt) (int destaddr, int w, int h, int pitch, int c);
    void (*hlinelistblt) (int ymin, int n, int *xmin, int *xmax, int pitch, int c);
    void (*bltwait) (void);
/* Other functions. */
    int (*ext_set) (unsigned what, va_list params);
    int (*accel) (unsigned operation, va_list params);
    int (*linear) (int op, int param);
    AccelSpecs *accelspecs;
    Emulation *emul;
} DriverSpecs;

extern DriverSpecs *__svgalib_driverspecs;

#endif //_VGA_DRIVER_H
