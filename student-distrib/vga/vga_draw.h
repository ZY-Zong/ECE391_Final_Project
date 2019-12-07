//
// Created by liuzikai on 12/6/19.
//

#ifndef _VGA_DRAW_H
#define _VGA_DRAW_H

typedef unsigned int vga_rgb;
typedef unsigned int vga_argb;

void vga_set_color_argb(vga_argb color);
void vga_draw_pixel(int x, int y);
vga_rgb vga_get_pixel(int x, int y);

void vga_print_char(int x, int y, char ch, vga_argb fg, vga_argb bg);

#endif //_VGA_DRAW_H
