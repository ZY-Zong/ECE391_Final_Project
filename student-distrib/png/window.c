//
// Created by Zhenyu Zong on 2019/12/9.
//

#include "window.h"

/*
 * Assume the terminal is printed at (x, y). Window status bar should start at (x-6, y-21).
 * Window block consists of:
 *   |  name       |   size   |    position    |
 *      up           652 * 21    (x-6, y-21 )
 *      down         652 *  4    (x-6, y+480)
 *      left         6  * 480    (x-6, y    )
 *      right        6  * 480    (x+640, y  )
 *      red_b        11 *  12    (x+4,  y-17)
 *      yellow_b     12 *  12    (x+22, y-17)
 *      green_b      11 *  12    (x+41, y-17)
 *      yellow_b_c   12 *  12    (x+22, y-17)
 *      green_b_c    11 *  12    (x+41, y-17)
 */

static unsigned char png_file_buf[MAX_PNG_SIZE];
static unsigned char png_test_buf[MAX_PNG_SIZE];

void draw_png(const uint8_t *fname, int x_offset, int y_offset) {
    dentry_t test_png;
    int32_t readin_size;

    // Open the png file and read it into buffer
    int32_t read_png = read_dentry_by_name(fname, &test_png);
    if (0 == read_png) {
        readin_size = read_data(test_png.inode_num, 0, png_file_buf, sizeof(png_file_buf));
        if (readin_size == sizeof(png_test_buf)) {
            DEBUG_ERR("PNG SIZE NOT ENOUGH!");
        }
    }

    upng_t upng;
    unsigned width;
    unsigned height;
    unsigned px_size;
    const unsigned char *buffer;
    vga_argb c;
    unsigned idx;

    upng = upng_new_from_file(png_file_buf, (long) readin_size);
    upng.buffer = (unsigned char *) &png_test_buf;
    upng_decode(&upng);
    if (upng_get_error(&upng) == UPNG_EOK) {
        width = upng_get_width(&upng);
        height = upng_get_height(&upng);
        px_size = upng_get_pixelsize(&upng) / 8;
        printf("px_size = %u\n", px_size);
        buffer = upng_get_buffer(&upng);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                idx = (j * width + i) * px_size;
                c = buffer[idx + 3] << 24 | buffer[idx + 0] << 16 | buffer[idx + 1] << 8 | buffer[idx + 2];
                vga_set_color_argb(c);
                vga_draw_pixel(i + x_offset, j + y_offset);
            }
        }
    }
}

void full_screen_png() {
    draw_png("background.png", 0, 0);
}

void draw_terminal_status_bar() {
    draw_png("up_window.png",    TERMINAL_X - 6,  TERMINAL_Y - 21 );
    draw_png("left_window.png",  TERMINAL_X - 6,  TERMINAL_Y      );
    draw_png("down_window.png",  TERMINAL_X - 6,  TERMINAL_Y + 480);
    draw_png("right_window.png", TERMINAL_X +640, TERMINAL_Y      );
    draw_png("red_b.png",        TERMINAL_X + 4,  TERMINAL_Y - 17 );
    draw_png("yellow_b.png",     TERMINAL_X + 22, TERMINAL_Y - 17 );
    draw_png("green_b.png",      TERMINAL_X + 41, TERMINAL_Y - 17 );
}

void draw_pressed_yellow_button() {
    draw_png("yellow_b_c.png",   TERMINAL_X + 22, TERMINAL_Y - 17 );
}

void draw_pressed_green_button() {
    draw_png("green_b_c.png",    TERMINAL_X + 22, TERMINAL_Y - 17 );
}
