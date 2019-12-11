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

gui_object_t gui_obj_desktop;
gui_object_t gui_obj_win_up;
gui_object_t gui_obj_win_down;
gui_object_t gui_obj_win_left;
gui_object_t gui_obj_win_right;
gui_object_t gui_obj_red[2];
gui_object_t gui_obj_yellow[2];
gui_object_t gui_obj_green[2];

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
int load_png(const char *fname, unsigned char *canvas, int x_offset, int y_offset,
             gui_object_t *gui_object, int expected_width, int expected_height) {
    dentry_t test_png;
    int32_t readin_size;

    // Open the png file and read it into buffer
    int32_t read_png = read_dentry_by_name((const uint8_t *) fname, &test_png);
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
        if (width != expected_width) {
            DEBUG_WARN("draw_png(): %s does not meet expected width", fname);
        }
        height = upng_get_height(&upng);
        if (height != expected_height) {
            DEBUG_WARN("draw_png(): %s does not meet expected height", fname);
        }
        px_size = upng_get_pixelsize(&upng) / 8;
        buffer = upng_get_buffer(&upng);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                idx = (j * width + i) * px_size;
                if (px_size == 3) {
                    c = 0xFF << 24 | buffer[idx + 0] << 16 | buffer[idx + 1] << 8 | buffer[idx + 2];
                } else if (px_size == 4) {
                    c = buffer[idx + 3] << 24 | buffer[idx + 0] << 16 | buffer[idx + 1] << 8 | buffer[idx + 2];
                } else {
                    DEBUG_ERR("draw_png(): invalid pixel size %d", px_size);
                    return -1;
                }
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

        gui_object->canvas = canvas;
        gui_object->x = x_offset;
        gui_object->y = y_offset;
        gui_object->width = width;
        gui_object->height = height;
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

gui_object_t gui_get_obj_font(char ch) {
    return (gui_object_t) {NULL,
                           (ch & 0x7F) * FONT_WIDTH, VGA_HEIGHT * 2 + (ch / 128) * FONT_HEIGHT,
                           FONT_WIDTH, FONT_HEIGHT};
}

static void init_desktop_obj() {
    ca_clear(desktop_canvas, sizeof(desktop_canvas));
    load_png(GUI_DESKTOP_FILENAME, desktop_canvas, 0, 0, &gui_obj_desktop, VGA_WIDTH, VGA_HEIGHT);
}

#define WIN_UP_X       (0)
#define WIN_UP_Y       (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_DOWN_X     (0)
#define WIN_DOWN_Y     (VGA_HEIGHT * 2 + FONT_HEIGHT * 2 + 25)
#define WIN_LEFT_X     (VGA_WIDTH - 16)
#define WIN_LEFT_Y     (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_RIGHT_X    (VGA_WIDTH - 8)
#define WIN_RIGHT_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)

#define WIN_RED_B_X         (16 * 41)
#define WIN_RED_B_Y         (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_YELLOW_B_X      (16 * 41 + 16)
#define WIN_YELLOW_B_Y      (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_GREEN_B_X       (16 * 41 + 16 + 16)
#define WIN_GREEN_B_Y       (VGA_HEIGHT * 2 + FONT_HEIGHT * 2)
#define WIN_YELLOW_B_C_X    (16 * 41 + 16)
#define WIN_YELLOW_B_C_Y    (VGA_HEIGHT * 2 + FONT_HEIGHT * 2 + 16)
#define WIN_GREEN_B_C_X     (16 * 41 + 16 + 16)
#define WIN_GREEN_B_C_Y     (VGA_HEIGHT * 2 + FONT_HEIGHT * 2 + 16)

static void init_window_obj() {
    load_png("up_window.png", NULL, WIN_UP_X, WIN_UP_Y,
             &gui_obj_win_up, 652, 21);
    load_png("down_window.png", NULL, WIN_DOWN_X, WIN_DOWN_Y,
             &gui_obj_win_down, 652, 4);
    load_png("left_window.png", NULL, WIN_LEFT_X, WIN_LEFT_Y,
             &gui_obj_win_left, 6, 480);
    load_png("right_window.png", NULL, WIN_RIGHT_X, WIN_RIGHT_Y,
             &gui_obj_win_right, 6, 480);

    load_png("red_b.png", NULL, WIN_RED_B_X, WIN_RED_B_Y,
             &gui_obj_red[0], 11, 12);
    load_png("yellow_b.png", NULL, WIN_YELLOW_B_X, WIN_YELLOW_B_Y,
             &gui_obj_yellow[0], 12, 12);
    load_png("green_b.png", NULL, WIN_GREEN_B_X, WIN_GREEN_B_Y,
             &gui_obj_green[0], 11, 12);

    // FIXME: normal red button?
    load_png("red_b.png", NULL, WIN_RED_B_X, WIN_RED_B_Y,
             &gui_obj_red[1], 11, 12);
    load_png("yellow_b_c.png", NULL, WIN_YELLOW_B_C_X, WIN_YELLOW_B_C_Y,
             &gui_obj_yellow[1], 12, 12);
    load_png("green_b_c.png", NULL, WIN_GREEN_B_C_X, WIN_GREEN_B_C_Y,
             &gui_obj_green[1], 11, 12);
}

void gui_obj_load() {
    init_font_obj(GUI_FONT_FORECOLOR_ARGB, GUI_FONT_BACKCOLOR_ARGB);
    init_desktop_obj();
    init_window_obj();
}