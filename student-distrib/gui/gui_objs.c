//
// Created by liuzikai on 12/7/19.
//

#include "gui_objs.h"

#include "gui_font_data.h"

static void init_font_objects(vga_rgb fg, vga_rgb bg) {

    int ch;
    int i, j;
    int x, y;

    for (ch = 0; ch < 256; ch++) {

        x = (ch & 0x7F) * FONT_WIDTH;
        y = VGA_HEIGHT + (ch / 128) * FONT_HEIGHT;

        for (i = 0; i < FONT_WIDTH; i++) {
            for (j = 0; j < FONT_HEIGHT; j++) {
                vga_set_color_argb((font_data[ch][j] & (1 << (7 - i)) ? fg : bg));
                vga_draw_pixel(x + i, y + j);
            }
        }
    }
}

void gui_obj_load() {
    init_font_objects(GUI_FONT_FORECOLOR_ARGB, GUI_FONT_BACKCOLOR_ARGB);
}

gui_object_t gui_obj_font(char ch) {
    gui_object_t ret = {(ch & 0x7F) * FONT_WIDTH, VGA_HEIGHT + (ch / 128) * FONT_HEIGHT,
                           FONT_WIDTH, FONT_HEIGHT};
    return ret;
}