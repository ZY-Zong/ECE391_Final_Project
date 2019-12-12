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
#if VGA_DRAW_ALPHA_BLEND
    gr_writew(color_convert(rgb_blend(color_revert(gr_readw(offset)), curr_color, alpha(curr_color))), offset);
#else
    gr_writew(color_convert(curr_color), offset);
#endif
}

void vga_set_byte(unsigned long offset, unsigned char b) {
    vga_set_page(offset >> 16);
    offset &= 0xFFFF;
    gr_writeb(b, offset);
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

void vga_draw_img(const vga_argb *img_data, unsigned width, unsigned height, int start_x, int start_y) {
    unsigned long offset;
    vga_argb c;
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {

            c = img_data[y * width + x];

            offset = (start_y + y) * vga_info.xbytes + (start_x + x) * VGA_BYTES_PER_PIXEL;
            vga_set_page(offset >> 16);
            offset &= 0xFFFF;

#if VGA_DRAW_ALPHA_BLEND
            gr_writew(color_convert(rgb_blend(color_revert(gr_readw(offset)), c, alpha(c))), offset);
#else
            gr_writew(color_convert(c), offset);
#endif
        }
    }
}