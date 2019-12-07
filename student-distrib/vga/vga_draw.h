//
// Created by liuzikai on 12/6/19.
//

#ifndef _VGA_DRAW_H
#define _VGA_DRAW_H

typedef unsigned int vga_rgb;
typedef unsigned int vga_argb;
typedef unsigned short vga_color;  // 16-bit RRRRRGGGGGGBBBBB color


#define alpha(argb)    (((argb) >> 24) & 0xFF)

static inline unsigned int channel_blend(unsigned int a, unsigned int b, unsigned int alpha) {
    unsigned int c1 = ((0x100 - alpha) * (a & 0xFF)) >> 8;
    unsigned int c2 = (alpha * (b & 0xFF)) >> 8;
    return ((c1 | c2) & 0xFF);
}

static inline vga_rgb rgb_blend(vga_rgb colora, vga_rgb colorb, unsigned int alpha) {
    unsigned int rb1 = ((0x100 - alpha) * (colora & 0xFF00FF)) >> 8;
    unsigned int rb2 = (alpha * (colorb & 0xFF00FF)) >> 8;
    unsigned int g1 = ((0x100 - alpha) * (colora & 0x00FF00)) >> 8;
    unsigned int g2 = (alpha * (colorb & 0x00FF00)) >> 8;
    return ((rb1 | rb2) & 0xFF00FF) + ((g1 | g2) & 0x00FF00);
}

static inline vga_color rgb_to_color(vga_rgb rgb) {
    return (((rgb >> 3) & 0x1F) | ((rgb >> 5) & 0x7E0) | ((rgb >> 8) & 0xF800));
}

static inline vga_rgb color_to_rgb (vga_color color) {
    vga_color c = color;
    return (((c & 0x1F) << 3) | ((c & 0x7E0) << 5) | ((c & 0xF800) << 8));
}


void vga_set_color_argb(vga_argb color);
void vga_draw_pixel(int x, int y);
vga_rgb vga_get_pixel(int x, int y);

#endif //_VGA_DRAW_H
