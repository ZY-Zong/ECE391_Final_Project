//
// Created by liuzikai on 12/6/19.
//

#include "vga_draw.h"
#include "vga.h"

static inline int GRB2RGB(int c) {
/* a bswap would do the same as the first 3 but in only ONE! cycle. */
/* However bswap is not supported by 386 */

    c = (c & 0xFF) |
        ((c >> 8) & 0xFF) << 16 |
        ((c >> 16) & 0xFF) << 8;

    return c;
}

static vga_argb curr_color = 0;

void vga_set_color_argb(vga_argb color) {
    curr_color = color;
}

/**
 * Draw a pixel using color set by vga_set_color_argb()
 * @param x    X coordinate of pixel
 * @param y    Y coordinate of pixel
 */
void vga_draw_pixel(int x, int y) {
    unsigned long offset = y * vga_info.xbytes + x * VGA_BYTES_PER_PIXEL;
    vga_set_page(offset >> 16);
    offset &= 0xFFFF;
//    gr_writew(rgb_to_color(rgb_blend(color_to_rgb(gr_readw(offset)), curr_color, alpha(curr_color))), offset);
//    gr_writeb(rgb_to_color(curr_color) >> 8, offset);
//    gr_writeb(rgb_to_color(curr_color) , offset + 1);


//            offset = y * vga_info.xbytes + x * 2;
//
//            vga_set_page(offset >> 16);
//
            gr_writew(0x1F, offset & 0xffff);

}

/**
 * Get a pixel rgb
 * @param x    X coordinate of pixel
 * @param y    Y coordinate of pixel
 * @return RGB color of the pixel
 */
vga_rgb vga_get_pixel(int x, int y) {
    unsigned long offset = y * vga_info.xbytes + x * VGA_BYTES_PER_PIXEL;
    vga_set_page(offset >> 16);
    return color_to_rgb(gr_readw(offset & 0xFFFF));
}