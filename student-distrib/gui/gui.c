//
// Created by liuzikai on 12/7/19.
//

#include "gui.h"
#include "gui_font_data.h"

static int curr_y = 0;

static void inline draw_object(gui_object_t *obj, int x, int y) {
    y += curr_y;  // double buffering
    if (obj->canvas == NULL) {
        vga_screen_copy(obj->x, obj->y, x, y, obj->width, obj->height);
    } else {
        vga_buf_copy((unsigned int *) obj->canvas, x, y, obj->width, obj->height);
    }
}

static void inline print_char(char ch, int x, int y) {
    gui_object_t c = gui_get_obj_font(ch);
    draw_object(&c, x, y);
}

static void draw_desktop() {
    draw_object(&gui_obj_desktop, 0, 0);
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
 */

static void draw_window_border(int terminal_x, int terminal_y, int r, int y, int g) {
    draw_object(&gui_obj_win_up, terminal_x - 6, terminal_y - 21);
    draw_object(&gui_obj_win_left, terminal_x - 6, terminal_y);
    draw_object(&gui_obj_win_down, terminal_x - 6, terminal_y + 480);
    draw_object(&gui_obj_win_right, terminal_x + 640, terminal_y);
    draw_object(&gui_obj_red[r], terminal_x + 4, terminal_y - 17);
    draw_object(&gui_obj_yellow[y], terminal_x + 22, terminal_y - 17);
    draw_object(&gui_obj_green[g], terminal_x + 41, terminal_y - 17);
}

static void draw_terminal_content(const char *buf, int rows, int columns, int terminal_x, int terminal_y) {
    int x, y;
    for (y = 0; y < rows; y++) {
        for (x = 0; x < columns; x++) {
            print_char(buf[(y - 1) * columns + x],
                           x * FONT_WIDTH + terminal_x, (y - 1) * FONT_HEIGHT + terminal_y);
        }
    }
}

void gui_render() {
    uint32_t flags;
    cli_and_save(flags);
    {
//        curr_y = VGA_HEIGHT - curr_y;  // switch place to draw (double buffering)

        draw_desktop();
        draw_window_border(40, 50, 0, 0, 0);
        draw_terminal_content((const char *) screen_char, MAX_ROWS, MAX_COLS, 40, 50);

//        vga_set_start_addr(curr_y * VGA_BYTES_PER_LINE);
    }
    restore_flags(flags);
}