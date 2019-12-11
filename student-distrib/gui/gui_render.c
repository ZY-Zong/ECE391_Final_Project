//
// Created by liuzikai on 12/7/19.
//

#include "gui_render.h"

#include "gui.h"
#include "gui_font_data.h"
#include "../vga/vga.h"

static int curr_y = 0;

static void inline draw_object(gui_object_t *obj, int x, int y) {
    y += curr_y;  // double buffering
    if (obj->canvas == NULL) {
        if (obj->transparent_color) {
            vga_set_transparent(ENABLE_TRANSPARENCY_COLOR, color_convert(obj->transparent_color));
        } else {
            vga_set_transparent(DISABLE_TRANSPARENCY_COLOR, 0);
        }
        vga_screen_copy(obj->x, obj->y, x, y, obj->width, obj->height);
    } else {
        DEBUG_ERR("draw_object(): system-to-screen bitblt is still broken.");
//        vga_buf_copy((unsigned int *) obj->canvas, x, y, obj->width, obj->height);
    }
}

static void inline print_char(char ch, int x, int y) {
    gui_object_t c = gui_get_obj_font(ch);
    draw_object(&c, x, y);
}

static void draw_desktop() {
//    draw_object(&gui_obj_desktop, 0, 0);
    int page;
    for (page = 0; page < VGA_SCREEN_BYTES / VGA_PAGE_SIZE; page++) {
        vga_set_page(page + curr_y / 32);
        memcpy((void *) VIDEO, gui_obj_desktop.canvas + page * VGA_PAGE_SIZE, VGA_PAGE_SIZE);
    }
}

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
 *      grey_b       12 *  12    (x+4,  y-17) & (x+22, y-17) & (x+41, y-17)
 */

#define WIN_UP_BORDER_LEFT_MARGIN       6
#define WIN_UP_BORDER_UP_MARGIN         21
#define WIN_LEFT_BORDER_LEFT_MARGIN     6
#define WIN_DOWN_BORDER_LEFT_MARGIN     6
#define WIN_DOWN_BORDER_UP_MARGIN       TERMINAL_HEIGHT
#define WIN_RIGHT_BORDER_UP_MARGIN      TERMINAL_WIDTH
#define WIN_RED_B_LEFT_MARGIN           4
#define WIN_RED_B_UP_MARGIN             17
#define WIN_YELLOW_B_LEFT_MARGIN        22
#define WIN_YELLOW_B_UP_MARGIN          17
#define WIN_GREEN_B_LEFT_MARGIN         41
#define WIN_GREEN_B_UP_MARGIN           17

static void draw_window_border(int terminal_x, int terminal_y, int r, int y, int g) {
#if GUI_WINDOW_PNG_RENDER
    vga_draw_img(gui_win_up, WIN_UP_WIDTH, WIN_UP_HEIGHT, terminal_x - WIN_UP_BORDER_LEFT_MARGIN, terminal_y - WIN_UP_BORDER_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_left, WIN_LEFT_WIDTH, WIN_LEFT_HEIGHT, terminal_x - WIN_LEFT_BORDER_LEFT_MARGIN, terminal_y + curr_y);
    vga_draw_img(gui_win_down, WIN_DOWN_WIDTH, WIN_DOWN_HEIGHT, terminal_x - WIN_DOWN_BORDER_LEFT_MARGIN, terminal_y + WIN_DOWN_BORDER_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_right, WIN_RIGHT_WIDTH, WIN_RIGHT_HEIGHT, terminal_x + WIN_RIGHT_BORDER_UP_MARGIN, terminal_y + curr_y);
    vga_draw_img(gui_win_red[r], WIN_RED_B_WIDTH, WIN_RED_B_HEIGHT, terminal_x + WIN_RED_B_LEFT_MARGIN, terminal_y - WIN_RED_B_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_yellow[y], WIN_YELLOW_B_WIDTH, WIN_YELLOW_B_HEIGHT, terminal_x + WIN_YELLOW_B_LEFT_MARGIN, terminal_y - WIN_YELLOW_B_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_green[g], WIN_GREEN_B_WIDTH, WIN_GREEN_B_HEIGHT, terminal_x + WIN_GREEN_B_LEFT_MARGIN, terminal_y - WIN_GREEN_B_UP_MARGIN + curr_y);
#else
    draw_object(&gui_obj_win_up, terminal_x - WIN_UP_BORDER_LEFT_MARGIN, terminal_y - WIN_UP_BORDER_UP_MARGIN);
    draw_object(&gui_obj_win_left, terminal_x - WIN_LEFT_BORDER_LEFT_MARGIN, terminal_y);
    draw_object(&gui_obj_win_down, terminal_x - WIN_DOWN_BORDER_LEFT_MARGIN, terminal_y + WIN_DOWN_BORDER_UP_MARGIN);
    draw_object(&gui_obj_win_right, terminal_x + WIN_RIGHT_BORDER_UP_MARGIN, terminal_y);
    draw_object(&gui_obj_red[r], terminal_x + WIN_RED_B_LEFT_MARGIN, terminal_y - WIN_RED_B_UP_MARGIN);
    draw_object(&gui_obj_yellow[y], terminal_x + WIN_YELLOW_B_LEFT_MARGIN, terminal_y - WIN_YELLOW_B_UP_MARGIN);
    draw_object(&gui_obj_green[g], terminal_x + WIN_GREEN_B_LEFT_MARGIN, terminal_y - WIN_GREEN_B_UP_MARGIN);
#endif
}

static void draw_terminal_content(const char *buf, int rows, int columns, int terminal_x, int terminal_y) {
    int x, y;
    for (y = 0; y < rows; y++) {
        for (x = 0; x < columns; x++) {
            print_char(buf[y * columns + x],
                       x * FONT_WIDTH + terminal_x, y * FONT_HEIGHT + terminal_y);
        }
    }
}

void gui_render() {

    if (!gui_inited) return;

    static int terminal_x = 30;
    static int terminal_y = 30;

    uint32_t flags;
    cli_and_save(flags);
    {
        terminal_x += 1;
        if (terminal_x > VGA_WIDTH - 30 - CUR_TERMINAL_WIDTH) terminal_x = 30;
        terminal_y += 1;
        if (terminal_y > VGA_HEIGHT - 30 - CUR_TERMINAL_HEIGHT) terminal_y = 30;

        curr_y = VGA_HEIGHT - curr_y;  // switch place to draw (double buffering)

        draw_desktop();
        draw_window_border(terminal_x, terminal_y, 0, 0, 0);
        draw_terminal_content((const char *) screen_char, MAX_ROWS, MAX_COLS, terminal_x, terminal_y);

        vga_set_start_addr(curr_y * VGA_BYTES_PER_LINE);
    }
    restore_flags(flags);
}

