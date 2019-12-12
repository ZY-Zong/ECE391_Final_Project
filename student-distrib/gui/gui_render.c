//
// Created by liuzikai on 12/7/19.
//

#include "gui_render.h"

#include "../vga/vga.h"
#include "../terminal.h"

#include "gui.h"
#include "gui_font_data.h"
#include "gui_window.h"
#include "qsort.h"
#include "gui_objs.h"
#include "../vidmem.h"

static int curr_y = 0;  // double buffering y coordinate

// Drawing optimization related
int grid_count;
int grid_x[GUI_MAX_WINDOW_NUM * 2 + 2];
int grid_y[GUI_MAX_WINDOW_NUM * 2 + 2];
gui_window_t *grid[GUI_MAX_WINDOW_NUM * 2 + 1][GUI_MAX_WINDOW_NUM * 2 + 1];  // [x index][y index]

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

/**
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
static void draw_window_border(int terminal_x, int terminal_y, int r, int y, int g) {
#if GUI_WINDOW_PNG_RENDER
    vga_draw_img(gui_win_up, WIN_UP_WIDTH, WIN_UP_HEIGHT, terminal_x - WIN_UP_BORDER_LEFT_MARGIN,
                 terminal_y - WIN_UP_BORDER_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_left, WIN_LEFT_WIDTH, WIN_LEFT_HEIGHT, terminal_x - WIN_LEFT_BORDER_LEFT_MARGIN,
                 terminal_y + curr_y);
    vga_draw_img(gui_win_down, WIN_DOWN_WIDTH, WIN_DOWN_HEIGHT, terminal_x - WIN_DOWN_BORDER_LEFT_MARGIN,
                 terminal_y + WIN_DOWN_BORDER_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_right, WIN_RIGHT_WIDTH, WIN_RIGHT_HEIGHT, terminal_x + WIN_RIGHT_BORDER_LEFT_MARGIN,
                 terminal_y + curr_y);
    vga_draw_img(gui_win_red[r], WIN_RED_B_WIDTH, WIN_RED_B_HEIGHT, terminal_x + WIN_RED_B_LEFT_MARGIN,
                 terminal_y - WIN_RED_B_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_yellow[y], WIN_YELLOW_B_WIDTH, WIN_YELLOW_B_HEIGHT, terminal_x + WIN_YELLOW_B_LEFT_MARGIN,
                 terminal_y - WIN_YELLOW_B_UP_MARGIN + curr_y);
    vga_draw_img(gui_win_green[g], WIN_GREEN_B_WIDTH, WIN_GREEN_B_HEIGHT, terminal_x + WIN_GREEN_B_LEFT_MARGIN,
                 terminal_y - WIN_GREEN_B_UP_MARGIN + curr_y);
#else
    draw_object(&gui_obj_win_up, terminal_x - WIN_UP_BORDER_LEFT_MARGIN, terminal_y - WIN_UP_BORDER_UP_MARGIN);
    draw_object(&gui_obj_win_left, terminal_x - WIN_LEFT_BORDER_LEFT_MARGIN, terminal_y);
    draw_object(&gui_obj_win_down, terminal_x - WIN_DOWN_BORDER_LEFT_MARGIN, terminal_y + WIN_DOWN_BORDER_UP_MARGIN);
    draw_object(&gui_obj_win_right, terminal_x + WIN_RIGHT_BORDER_LEFT_MARGIN, terminal_y);

    if (r == -1) {
        draw_object(&gui_obj_grey, terminal_x + WIN_RED_B_LEFT_MARGIN, terminal_y - WIN_RED_B_UP_MARGIN);
    } else {
        draw_object(&gui_obj_red[r], terminal_x + WIN_RED_B_LEFT_MARGIN, terminal_y - WIN_RED_B_UP_MARGIN);
    }

    if (y == -1) {
        draw_object(&gui_obj_grey, terminal_x + WIN_YELLOW_B_LEFT_MARGIN, terminal_y - WIN_YELLOW_B_UP_MARGIN);
    } else {
        draw_object(&gui_obj_yellow[y], terminal_x + WIN_YELLOW_B_LEFT_MARGIN, terminal_y - WIN_YELLOW_B_UP_MARGIN);
    }

    if (g == -1) {
        draw_object(&gui_obj_grey, terminal_x + WIN_GREEN_B_LEFT_MARGIN, terminal_y - WIN_GREEN_B_UP_MARGIN);
    } else {
        draw_object(&gui_obj_green[g], terminal_x + WIN_GREEN_B_LEFT_MARGIN, terminal_y - WIN_GREEN_B_UP_MARGIN);
    }
#endif
}

/**
 * Starting goose ui:
 * |   name   |   size   |  position
 *   goose.png  190 * 164  (417, 301)
 *   GA!.png    104 * 59   (532,246),(308,393),(584,468)
 *
 */


static inline void draw_terminal_content(const char *buf, int buf_start_x, int buf_start_y, int buf_cols, int buf_rows,
                                  int term_x, int term_y) {
    int x, y;
    for (y = 0; y < buf_rows; y++) {
        for (x = 0; x < buf_cols; x++) {
            print_char(buf[(y + buf_start_y) * TERMINAL_TEXT_COLS + (x + buf_start_x)],
                       x * FONT_WIDTH + term_x, y * FONT_HEIGHT + term_y);
        }
    }
}

/**
 * Find the window that has (part of) its body (terminal) visible on the given point
 * @param x
 * @param y
 * @return
 */
static inline gui_window_t *find_visible_win_on(int x, int y) {
    // From top to bottom
    int i;
    for (i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
        if (check_inside_window(x, y, window_stack[i]) == IN_BODY) return window_stack[i];
    }
    return NULL;
}

#define ceil_div(x, y)     (((x) + (y) - 1) / (y))
#define floor_div(x, y)    ((x) / (y))

void gui_render() {

    if (!gui_inited) return;

    uint32_t flags;
    cli_and_save(flags);
    {
        // Switch place to draw (double buffering)
        curr_y = VGA_HEIGHT - curr_y;

        // Open all buffer
        terminal_vidmem_set(NULL_TERMINAL_ID);

        /// Render desktop and status bar
        draw_desktop();

        /// Render Window

        gui_window_t *win;

        // Generate grids
        grid_count = 1;
        grid_x[0] = 0;
        grid_y[0] = 0;
        grid_x[1] = VGA_WIDTH;
        grid_y[1] = VGA_HEIGHT;
        int i;
        for (i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
            win = window_stack[i];
            if (win != NULL) {
                grid_x[grid_count + 1] = win->term_x;
                grid_y[grid_count + 1] = win->term_y;
                grid_x[grid_count + 2] = win->term_x + TERMINAL_WIDTH_PIXEL;
                grid_y[grid_count + 2] = win->term_y + TERMINAL_HEIGHT_PIXEL;
                grid_count += 2;
            }
        }
        quick_sort(grid_x, 0, grid_count);
        quick_sort(grid_y, 0, grid_count);

        // Fill grids
        int idx, idy;
        for (idx = 0; idx < grid_count; idx++) {
            for (idy = 0; idy < grid_count; idy++) {
                win = find_visible_win_on(grid_x[idx], grid_y[idy]);
                grid[idx][idy] = win;
                if (win != NULL) {
                    int buf_start_x = (grid_x[idx] - win->term_x) / FONT_WIDTH;
                    int buf_cols = ceil_div(grid_x[idx + 1] - grid_x[idx],  FONT_WIDTH);
                    int buf_start_y = (grid_y[idy] - win->term_y) / FONT_HEIGHT;
                    int buf_rows = ceil_div(grid_y[idy + 1] - grid_y[idy], FONT_HEIGHT);
                    draw_terminal_content((const char *) win->screen_char, buf_start_x, buf_start_y,
                                          buf_cols, buf_rows,
                                          win->term_x + buf_start_x * FONT_WIDTH,
                                          win->term_y + buf_start_y * FONT_HEIGHT);
                }
            }
        }

        // Draw the inactive window, from bottom to top, except the top window
        for (i = GUI_MAX_WINDOW_NUM - 1; i >= 1; i--) {
            win = window_stack[i];
            if (win != NULL) {
                for (idx = 0; idx < grid_count; idx++) {
                    for (idy = 0; idy < grid_count; idy++) {
                        if (grid[idx][idy] == win) {
                            int buf_start_x = (grid_x[idx] - win->term_x) / FONT_WIDTH;
                            int buf_cols = ceil_div(grid_x[idx + 1] - grid_x[idx],  FONT_WIDTH);
                            int buf_start_y = (grid_y[idy] - win->term_y) / FONT_HEIGHT;
                            int buf_rows = ceil_div(grid_y[idy + 1] - grid_y[idy], FONT_HEIGHT);
                            draw_terminal_content((const char *) win->screen_char, buf_start_x, buf_start_y,
                                                  buf_cols, buf_rows,
                                                  win->term_x + buf_start_x * FONT_WIDTH,
                                                  win->term_y + buf_start_y * FONT_HEIGHT);
                        }
                    }
                }
                draw_window_border(win->term_x, win->term_y, -1, -1, -1);
            }
        }

        // Draw the top window, which can't be covered by any window
        if (window_stack[0] != NULL) {
            win = window_stack[0];
            draw_terminal_content((const char *) win->screen_char, 0, 0,
                                  TERMINAL_TEXT_COLS, TERMINAL_TEXT_ROWS, win->term_x, win->term_y);
            draw_window_border(win->term_x, win->term_y, 0, 0, 0);
        }

        // Wait for BitBLT engine to complete
        vga_accel_sync();

        // Switch view
        vga_set_start_addr(curr_y * VGA_BYTES_PER_LINE);

        // Restore terminal mapping
        terminal_vidmem_set(running_term()->terminal_id);
    }
    restore_flags(flags);
}

