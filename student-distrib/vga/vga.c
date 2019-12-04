//
// Created by liuzikai on 12/3/19.
//

#include "vga.h"
#include "vga_port.h"

#include "../lib.h"

// TODO: change all port_out to outb and all port_in to inb

int prev_mode;
int curr_mode = TEXT;

// TODO: eliminate these variables?
// TODO: are these initial values correct?
int __svgalib_CRT_I = CRT_IM;		/* current CRT index register address */
int __svgalib_CRT_D = CRT_DM;		/* current CRT data register address */
int __svgalib_IS1_R = IS1_RM;		/* current input status register address */

// TODO: rename this variable
struct info CI;			/* current video parameters */
struct info CI_G1280_1024_16M = {1280, 1024, 1 << 24, 1280 * 3, 3};

static void setcoloremulation(void);

/**
 * Turn off screen for faster VGA memory access
 */
void vga_screenoff() {
    outb(0x01, SEQ_I);
    outb(inb(SEQ_D) | 0x20, SEQ_D);
}

/**
 * Turn on screen
 */
void vga_screenon() {
    outb(0x01, SEQ_I);
    outb(inb(SEQ_D) & 0xDF, SEQ_D);
}

/**
 * Transplanted function.
 * @param mode   Only support G1280x1024x16M (28)
 * @return
 */
int vga_set_mode(int mode) {

    if (mode != G1280x1024x16M) {
        DEBUG_ERR("vag_set_mode(): only support entering G1028x1024x16M mode.");
        return;
    }

    int mode_flags = mode & 0xfffff000;

    mode &= 0xfff;

    if (!initialized)
        initialize();

    unsigned int interrupt_flags;
    cli_and_save(interrupt_flags);
    // TODO: restore_flags(interrupt_flags)

    prev_mode = curr_mode;
    curr_mode = mode;

    /* disable video */
    vga_screenoff();



        /* Setting a graphics mode. */

        /* disable text output */
        /*if (!__svgalib_secondary)
            ioctl(__svgalib_tty_fd, KDSETMODE, KD_GRAPHICS);*/

        /* shift to color emulation */
        setcoloremulation();

        CI = CI_G1280_1024_16M;

        // TODO: MARK
        chipset_setmode(mode, prv_mode);
        MODEX = 0;

        /* Set default claimed memory (moved here from initialize - Michael.) */
        if (mode == G320x200x256)
            VMEM = 65536;
        else if (STDVGAMODE(mode))
            VMEM = 256 * 1024;    /* Why always 256K ??? - Michael */
        else {
            vga_modeinfo *modeinfo;

            modeinfo = vga_getmodeinfo(mode);
            VMEM = modeinfo->linewidth * modeinfo->height;
            CI.xbytes = modeinfo->linewidth;
        }

        if (!flip) {
            /* set default palette */
            if (CI.colors <= 256)
                restorepalette(default_red, default_green, default_blue);

            /* clear screen (sets current color to 15) */
            __svgalib_currentpage = -1;
            if (!(mode_flags & 0x8000))vga_clear();

            if (SVGAMODE(__svgalib_cur_mode))
                vga_setpage(0);
        }
        __svgalib_currentpage = -1;
        currentlogicalwidth = CI.xbytes;
        currentdisplaystart = 0;

        usleep(MODESWITCHDELAY);    /* wait for signal to stabilize */

        /* enable video */
        if (!flip)
            vga_screenon();

        if (mouse_support && mouse_open) {
            /* vga_lockvc(); */
            mouse_setxrange(0, CI.xdim - 1);
            mouse_setyrange(0, CI.ydim - 1);
            mouse_setwrap(MOUSE_NOWRAP);
            mouse_mode = mode;
        }
        {
            vga_modeinfo *modeinfo;
            modeinfo = vga_getmodeinfo(mode);
            MODEX = ((MODEFLAGS = modeinfo->flags) & IS_MODEX);
        }

        if (!flip)
            /* set graphics mode termio */
            set_graphtermio();
        else if (__svgalib_kbd_fd < 0)
            enable_interrupt();
    }

/*    if (!flip)
   vga_unlockvc(); */
#ifdef BACKGROUND
    __svgalib_is_vt_switching_needed();
#endif
    return 0;
}

static void setcoloremulation(void)
{
    /* shift to color emulation */
    __svgalib_CRT_I = CRT_IC;
    __svgalib_CRT_D = CRT_DC;
    __svgalib_IS1_R = IS1_RC;
    outb(inb(MIS_R) | 0x01, MIS_W);
}