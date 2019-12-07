//
// Created by liuzikai on 12/6/19.
//

#include "vga_draw.h"
#include "vga.h"

static vga_argb curr_color = 0;

void vga_set_color_argb(vga_argb color) {
    curr_color = color;
}

/**
 * Draw a pixel using color set by vga_set_color_rgb() or vga_set_color_argb()
 * @param x    X coordinate of pixel
 * @param y    Y coordinate of pixel
 * @return 0 for success, other values for failure
 */
void vga_draw_pixel(int x, int y) {
    unsigned long offset = y * vga_info.xbytes + x * VGA_BYTES_PER_PIXEL;
    vga_set_page(offset >> 16);
    offset &= 0xFFFF;
//    gr_writew(rgb_to_color(curr_color), offset);
    gr_writew(rgb_to_color(rgb_blend(color_to_rgb(gr_readw(offset)), curr_color, alpha(curr_color))), offset);

}

vga_rgb vga_get_pixel(int x, int y) {
    unsigned long offset = y * vga_info.xbytes + x * VGA_BYTES_PER_PIXEL;
    vga_set_page(offset >> 16);
    return color_to_rgb(gr_readw(offset & 0xFFFF));
}