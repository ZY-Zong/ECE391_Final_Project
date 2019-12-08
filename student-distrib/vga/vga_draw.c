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
 * Draw a pixel using color set by vga_set_color_argb()
 * @param x    X coordinate of pixel
 * @param y    Y coordinate of pixel
 */
void vga_draw_pixel(int x, int y) {
    unsigned long offset = y * vga_info.xbytes + x * VGA_BYTES_PER_PIXEL;
    vga_set_page(offset >> 16);
    offset &= 0xFFFF;
    gr_writew(color_convert(rgb_blend(color_revert(gr_readw(offset)), curr_color, alpha(curr_color))), offset);
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
    return color_revert(gr_readw(offset & 0xFFFF));
}