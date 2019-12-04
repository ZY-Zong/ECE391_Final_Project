//
// Created by liuzikai on 12/4/19.
//

#include "vga_pixel.h"

static inline int RGB2BGR(int c)
{
/* a bswap would do the same as the first 3 but in only ONE! cycle. */
/* However bswap is not supported by 386 */

    // TODO: assume not
//    if (MODEFLAGS & RGB_MISORDERED)
//            asm("rorw  $8, %0\n"	/* 0RGB -> 0RBG */
//                "rorl $16, %0\n"	/* 0RBG -> BG0R */
//                "rorw  $8, %0\n"	/* BG0R -> BGR0 */
//                "shrl  $8, %0\n"	/* 0BGR -> 0BGR */
//            : "=q"(c):"0"(c));

    return c;
}

int vga_drawpixel(int x, int y) {
    unsigned long offset;
    int c;

    // TODO: change to current color
    c = 0x00FF00;
    offset = y * CI.xbytes + x * 3;

    vga_setpage(offset >> 16);

    switch (offset & 0xffff) {
        case 0xfffe:
            gr_writew(c, 0xfffe);

            vga_setpage((offset >> 16) + 1);

            gr_writeb(c >> 16, 0);
            break;
        case 0xffff:
            gr_writeb(c, 0xffff);

            vga_setpage((offset >> 16) + 1);

            gr_writew(c >> 8, 0);
            break;
        default:
            offset &= 0xffff;
            gr_writew(c, offset);
            gr_writeb(c >> 16, offset + 2);
            break;
    }

    return 0;
}