//
// Created by liuzikai on 12/6/19.
//

#ifndef _VGA_DRAW_H
#define _VGA_DRAW_H

typedef unsigned int vga_rgb;
typedef unsigned int vga_argb;     // 32-bit RRRRRRRRGGGGGGGGBBBBBBBB
typedef unsigned short vga_color;  // 15-bit RRRRRGGGGGBBBBB

#define VGA_DRAW_ALPHA_BLEND    0

#define alpha(argb)    (((argb) >> 24) & 0xFF)

/**
 * Alpha blend on channels
 * @param a        Background channel. Any bits except low 8 bits are discarded.
 * @param b        Foreground channel. Any bits except low 8 bits are discarded.
 * @param alpha    Alpha value of foreground
 * @return Result channel
 */
static inline unsigned int channel_blend(unsigned int a, unsigned int b, unsigned int alpha) {
    unsigned int c1 = ((0x100 - alpha) * (a & 0xFF)) >> 8;
    unsigned int c2 = (alpha * (b & 0xFF)) >> 8;
    return ((c1 | c2) & 0xFF);
}

/**
 * Alpha blend on RGB colors
 * @param colora    Background Color. High 8 bits are safely discarded.
 * @param colorb    Foreground Color. High 8 bits are safely discarded.
 * @param alpha     Alpha value of foreground
 * @return Result RGB
 */
static inline vga_rgb rgb_blend(vga_rgb colora, vga_rgb colorb, unsigned int alpha) {
    unsigned int rb1 = ((0x100 - alpha) * (colora & 0xFF00FF)) >> 8;
    unsigned int rb2 = (alpha * (colorb & 0xFF00FF)) >> 8;
    unsigned int g1 = ((0x100 - alpha) * (colora & 0x00FF00)) >> 8;
    unsigned int g2 = (alpha * (colorb & 0x00FF00)) >> 8;
    return ((rb1 | rb2) & 0xFF00FF) + ((g1 | g2) & 0x00FF00);
}

/**
 * Convert RGB color to 16-bit VGA color
 * @param rgb    RGB color to be convert. High 8 bits are safely discarded.
 * @return Result 16-bit VGA color
 */
static inline vga_color color_convert(vga_rgb rgb) {
    return ((rgb >> 3) & 0x1F) |  // blue
           ((rgb >> 6) & 0x3E0) |    // green
           ((rgb >> 9) & 0x7C00);         // red
}

/**
 * Convert 16-bit VGA color to RGB color
 * @param color    16-bit VGA color.
 * @return Result 16-bit VGA color. First 8 bits are 0.
 */
static inline vga_rgb color_revert(vga_color c) {
    return ((vga_rgb) c & 0x1F) << 3 |          // blue
           ((vga_rgb) c & 0x3E0) << 6 |  // green
           ((vga_rgb) c & 0x7C00) << 9;     // blue
}


void vga_set_color_argb(vga_argb color);
void vga_draw_pixel(int x, int y);
vga_rgb vga_get_pixel(int x, int y);
void vga_draw_img(const vga_argb *img_data, unsigned width, unsigned height, int start_x, int start_y);

#endif //_VGA_DRAW_H
