//
// Created by liuzikai on 12/7/19.
//

#include "gui_objs.h"

#include "gui_font_data.h"

#define FONT_CANVAS_INDEX    0

static char *font_ca = (char *) 0x000B9003;

#define ca_readb(canvas, off)        (((volatile unsigned char *)canvas)[(off)])
#define ca_readw(canvas, off)        (*(volatile unsigned short*)((canvas)+(off)))
#define ca_readl(canvas, off)        (*(volatile unsigned long*)((canvas)+(off)))
#define ca_writeb(canvas, v, off)    (canvas[(off)] = (v))
#define ca_writew(canvas, v, off)    (*(unsigned short*)((canvas)+(off)) = (v))
#define ca_writel(canvas, v, off)    (*(unsigned long*)((canvas)+(off)) = (v))

static void init_font_objects(vga_rgb fg, vga_rgb bg) {
    int ch;
    for (ch = 0; ch < 256; ch++) {
        vga_print_char((ch & 0x7F) * FONT_WIDTH, VGA_HEIGHT + (ch / 128) * FONT_HEIGHT,
                       ch, GUI_FONT_FORECOLOR_ARGB, GUI_FONT_BACKCOLOR_ARGB);
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