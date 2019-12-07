//
// Created by liuzikai on 12/6/19.
//

#ifndef _VGA_DRAW_H
#define _VGA_DRAW_H

typedef unsigned int vga_rgb;
typedef unsigned int vga_argb;
typedef unsigned short vga_color;  // 16-bit RRRRRGGGGGGBBBBB color

void vga_set_color_argb(vga_argb color);
void vga_draw_pixel(int x, int y);
vga_rgb vga_get_pixel(int x, int y);

void vga_print_char(int x, int y, char ch, vga_argb fg, vga_argb bg);
void vga_print_char_array(int start_x, int start_y, char* array, int array_rows, int array_columns, vga_argb fg, vga_argb bg);

#endif //_VGA_DRAW_H
