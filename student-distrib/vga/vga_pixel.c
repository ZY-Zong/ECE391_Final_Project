//
// Created by liuzikai on 12/4/19.
//

#include "vga_pixel.h"

#include "vga.h"

#define gr_readb(off)		(((volatile unsigned char *)VGA_GM)[(off)])
#define gr_readw(off)		(*(volatile unsigned short*)((VGA_GM)+(off)))
#define gr_readl(off)		(*(volatile unsigned long*)((VGA_GM)+(off)))
#define gr_writeb(v,off)	(VGA_GM[(off)] = (v))
#define gr_writew(v,off)	(*(unsigned short*)((VGA_GM)+(off)) = (v))
#define gr_writel(v,off)	(*(unsigned long*)((VGA_GM)+(off)) = (v))

int curr_color = 0;

void vga_setcolor(int rgb) {
    curr_color = rgb;
}

int vga_drawpixel(int x, int y) {

    unsigned long offset = y * vga_info.xbytes + x * 3;
    int c = curr_color;

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
