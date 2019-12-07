//
// Created by liuzikai on 12/7/19.
//

#include "gui_objs.h"

#include "gui_font_data.h"

#define FONT_CANVAS_INDEX    0

static char canvas[1][VGA_SCREEN_BYTES] __attribute__((aligned(4096)));

#define ca_readb(canvas, off)        (((volatile unsigned char *)canvas)[(off)])
#define ca_readw(canvas, off)        (*(volatile unsigned short*)((canvas)+(off)))
#define ca_readl(canvas, off)        (*(volatile unsigned long*)((canvas)+(off)))
#define ca_writeb(canvas, v, off)    (canvas[(off)] = (v))
#define ca_writew(canvas, v, off)    (*(unsigned short*)((canvas)+(off)) = (v))
#define ca_writel(canvas, v, off)    (*(unsigned long*)((canvas)+(off)) = (v))

static void init_font_objects(vga_rgb fg, vga_rgb bg) {

    int i, j;
    int start_x, start_y;
    int ch;
    unsigned long offset = 0;

    vga_rgb c;

    for (ch = 0; ch < 256; ch++) {

        start_x = (ch & 128) * FONT_WIDTH;
        start_y = (ch / 128) * FONT_HEIGHT;

        for (i = 0; i < FONT_WIDTH; i++) {
            for (j = 0; j < FONT_HEIGHT; j++) {

                c = ((font_data[ch][j] & (1 << (7 - i)) ? fg : bg));

                ca_writew(canvas[FONT_CANVAS_INDEX], c, offset);
                ca_writeb(canvas[FONT_CANVAS_INDEX], c >> 16, offset + 2);

                offset += VGA_BYTES_PER_PIXEL;
            }
        }
    }

    memset(canvas, 0xEE, sizeof(canvas));
}

void gui_obj_load() {
    init_font_objects(GUI_FONT_FORECOLOR, GUI_FONT_BACKCOLOR);
}

gui_object_t gui_obj_font(char ch) {
    gui_object_t ret;
    ret.src_addr = (char *) &canvas[FONT_CANVAS_INDEX] + (ch * FONT_WIDTH * FONT_HEIGHT) * VGA_BYTES_PER_PIXEL;
    ret.width = FONT_WIDTH;
    ret.height = FONT_HEIGHT;
    return ret;
}