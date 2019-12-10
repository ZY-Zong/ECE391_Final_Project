//
// Created by liuzikai on 12/7/19.
//

#include "gui_objs.h"

#include "gui_font_data.h"

#include "../file_system.h"
#include "upng.h"

static unsigned char png_file_buf[MAX_PNG_SIZE];
static unsigned char png_buf[MAX_PNG_SIZE];

static unsigned char desktop_canvas[VGA_SCREEN_BYTES];

void ca_clear(unsigned char *canvas, unsigned int size) {
    memset(canvas, 0, size);
}

void ca_draw_pixel(unsigned char *canvas, int x, int y, vga_argb argb) {
    unsigned long offset = y * vga_info.xbytes + x * VGA_BYTES_PER_PIXEL;
    offset &= 0xFFFF;
    *(unsigned short *) (canvas + offset) =
            color_convert(rgb_blend(color_revert(*(unsigned short *) (canvas + offset)),
                                    argb,
                                    ((argb >> 24) & 0xFF)));
}

/**
 * Load an png from file and draw it to video memory or canvas buffer
 * @param fname       PNG filename
 * @param canvas      If NULL, png will be drawn to video memory at x_offset and y_offset.
 *                    If not NULL, png will be drawn to this canvas
 * @param x_offset    Starting X if the image
 * @param y_offset    Starting Y if the image
 * @return 0 for success, -1 for failure
 */
int load_png(const char *fname, unsigned char *canvas, int x_offset, int y_offset) {
    dentry_t test_png;
    int32_t readin_size;

    // Open the png file and read it into buffer
    int32_t read_png = read_dentry_by_name(fname, &test_png);
    if (0 == read_png) {
        readin_size = read_data(test_png.inode_num, 0, png_file_buf, sizeof(png_file_buf));
        if (readin_size == sizeof(png_file_buf)) {
            DEBUG_ERR("draw_png(): PNG BUFFER NOT ENOUGH!");
            return -1;
        }
    } else {
        DEBUG_ERR("draw_png: png file %f does not exist!", fname);
        return -1;
    }

    upng_t upng;
    unsigned width;
    unsigned height;
    unsigned px_size;
    const unsigned char *buffer;
    vga_argb c;
    unsigned int idx;

    upng = upng_new_from_file(png_file_buf, (long) readin_size);
    upng.buffer = (unsigned char *) &png_buf;
    upng_decode(&upng);
    if (upng_get_error(&upng) == UPNG_EOK) {
        width = upng_get_width(&upng);
        height = upng_get_height(&upng);
        px_size = upng_get_pixelsize(&upng) / 8;
        buffer = upng_get_buffer(&upng);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                idx = (j * width + i) * px_size;
                c = buffer[idx + 3] << 24 | buffer[idx + 0] << 16 | buffer[idx + 1] << 8 | buffer[idx + 2];
                if (canvas == NULL) {
                    // Draw to video memory
                    vga_set_color_argb(c);
                    vga_draw_pixel(i + x_offset, j + y_offset);
                } else {
                    // Draw to canvas
                    ca_draw_pixel(canvas, i + x_offset, j + y_offset, c);
                }
            }
        }
    } else {
        DEBUG_ERR("draw_png: fail to decode png %s", fname);
        return -1;
    }

    return 0;
}

static void init_font_obj(vga_rgb fg, vga_rgb bg) {
    int ch;
    int i, j;
    int x, y;

    for (ch = 0; ch < 256; ch++) {

        x = (ch & 0x7F) * FONT_WIDTH;
        y = VGA_HEIGHT * 2 + (ch / 128) * FONT_HEIGHT;

        for (i = 0; i < FONT_WIDTH; i++) {
            for (j = 0; j < FONT_HEIGHT; j++) {
                vga_set_color_argb((font_data[ch][j] & (1 << (7 - i)) ? fg : bg));
                vga_draw_pixel(x + i, y + j);
            }
        }
    }
}

gui_object_t gui_obj_font(char ch) {
    gui_object_t ret = {NULL,
                        (ch & 0x7F) * FONT_WIDTH, VGA_HEIGHT * 2 + (ch / 128) * FONT_HEIGHT,
                        FONT_WIDTH, FONT_HEIGHT};
    return ret;
}

static void init_desktop_obj() {
    ca_clear(desktop_canvas, sizeof(desktop_canvas));
    load_png(GUI_DESKTOP_FILENAME, desktop_canvas, 0, 0);
}

gui_object_t gui_obj_desktop() {
    return (gui_object_t) {desktop_canvas, 0, 0, VGA_WIDTH, VGA_HEIGHT};
}

#define WIN_UP_X       (0)
#define WIN_UP_Y       (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_DOWN_X     (0)
#define WIN_DOWN_Y     (VGA_HEIGHT * 2 + FONT_HEIGHT * 2 + 25)
#define WIN_LEFT_X     (VGA_WIDTH - 16)
#define WIN_LEFT_Y     (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_RIGHT_X    (VGA_WIDTH - 8)
#define WIN_RIGHT_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)

#define WIN_RED_B_X    (16 * 41)
#define WIN_RED_B_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_YELLOW_B_X    (16 * 41 + 16)
#define WIN_YELLOW_B_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_GREEN_B_X    (16 * 41 + 16 + 16)
#define WIN_GREEN_B_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_YELLOW_B_C_X    (16 * 41 + 16)
#define WIN_YELLOW_B_C_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2 + 16)
#define WIN_GREEN_B_C_X    (16 * 41 + 16 + 16)
#define WIN_GREEN_B_C_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2 + 16)

static void init_window_obj() {
    load_png("up_window.png", NULL, WIN_UP_X, WIN_UP_Y);
    load_png("down_window.png", NULL, WIN_DOWN_X, WIN_DOWN_Y);
    load_png("left_window.png", NULL, WIN_LEFT_X, WIN_LEFT_Y);
    load_png("right_window.png", NULL, WIN_RIGHT_X, WIN_RIGHT_Y);

    load_png("red_b.png", NULL, WIN_RED_B_X, WIN_RED_B_Y);
    load_png("yellow_b.png", NULL, WIN_YELLOW_B_X, WIN_YELLOW_B_Y);
    load_png("green_b.png", NULL, WIN_GREEN_B_X, WIN_GREEN_B_Y);

    load_png("yellow_b_c.png", NULL, WIN_YELLOW_B_C_X, WIN_YELLOW_B_C_Y);
    load_png("green_b_c.png", NULL, WIN_GREEN_B_C_X, WIN_GREEN_B_C_Y);
}

gui_object_t gui_obj_win_up() {
    return (gui_object_t) {NULL, WIN_UP_X, WIN_UP_X, 652, 21};
}

gui_object_t gui_obj_win_down() {
    return (gui_object_t) {NULL, WIN_DOWN_X, WIN_DOWN_Y, 652, 4};
}

gui_object_t gui_obj_win_left() {
    return (gui_object_t) {NULL, WIN_LEFT_X, WIN_LEFT_Y, 6, 480};
}

gui_object_t gui_obj_win_right() {
    return (gui_object_t) {NULL, WIN_RIGHT_X, WIN_RIGHT_Y, 6, 480};
}

gui_object_t gui_obj_red(unsigned int pressed) {
    return (gui_object_t) {NULL, WIN_RED_B_X, WIN_RED_B_Y, 11, 12};
}

gui_object_t gui_obj_yellow(unsigned int pressed) {
    if (pressed) {
        return (gui_object_t) {NULL, WIN_YELLOW_B_C_X, WIN_YELLOW_B_C_Y, 12, 12};
    } else {
        return (gui_object_t) {NULL, WIN_YELLOW_B_X, WIN_YELLOW_B_Y, 12, 12};
    }
}

gui_object_t gui_obj_green(unsigned int pressed) {
    if (pressed) {
        return (gui_object_t) {NULL, WIN_GREEN_B_C_X, WIN_GREEN_B_C_Y, 11, 12};
    } else {
        return (gui_object_t) {NULL, WIN_GREEN_B_X, WIN_GREEN_B_Y, 11, 12};
    }
}

void gui_obj_load() {
    init_font_obj(GUI_FONT_FORECOLOR_ARGB, GUI_FONT_BACKCOLOR_ARGB);
    init_desktop_obj();
    init_window_obj();
}