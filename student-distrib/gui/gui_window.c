//
// Created by liuzikai on 12/10/19.
//

#include "gui_window.h"

#include "../lib.h"

gui_window_t *window_stack[GUI_MAX_WINDOW_NUM];
static int mouse_pressed = 0;

/**
 * Initialize GUI window control
 */
void gui_window_init() {
    for (int i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
        window_stack[i] = NULL;
    }
}

/**
 * Find a window in window stack
 * @param win    The window to find
 * @return Its index in window_stack, or -1 if it's not in the stack
 */
static int find_window_idx(const gui_window_t *win) {
    for (int i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
        if (window_stack[i] == win) return i;
    }
    return -1;
}

int gui_new_window(gui_window_t *win, char *screen_buf) {
    if (find_window_idx(win) != -1) {
        DEBUG_ERR("gui_new_window(): window already exists");
        return -1;
    }

    int idx = find_window_idx(NULL);

    if (idx == -1) {
        DEBUG_ERR("gui_new_window(): reach maximal window");
        return -1;
    }

    win->term_x = GUI_WIN_INITIAL_TERM_X;
    win->term_y = GUI_WIN_INITIAL_TERM_Y;
    win->screen_char = screen_buf;

    window_stack[idx] = win;

    return 0;
}

int gui_activate_window(gui_window_t *win) {
    int idx = find_window_idx(win);
    if (idx == -1) {
        DEBUG_ERR("gui_activate_window(): no such window");
        return -1;
    }

    // Put the window to the top of the stack
    for (int i = 1; i <= idx; i++) {
        window_stack[i] = window_stack[i - 1];
    }
    window_stack[0] = win;

    return 0;
}

int gui_destroy_window(gui_window_t* win) {
    int idx = find_window_idx(win);
    if (idx == -1) {
        DEBUG_ERR("gui_destroy_window(): no such window");
        return -1;
    }

    // Move all windows forward
    for (int i = idx; i < GUI_MAX_WINDOW_NUM - 1; i++) {
        window_stack[i] = window_stack[i + 1];
    }
    window_stack[GUI_MAX_WINDOW_NUM - 1] = NULL;

    return 0;
}

/**
 * Check whether the cursor is inside a window
 * @param x
 * @param y
 * @param win
 * @return 0 if the cursor is not in the window
 *         1 if the cursor is in the body of the window
 *         2 if the cursor is in the title bar of the window
 */
int check_inside_window(int x, int y, const gui_window_t* win) {

}

void gui_handle_mouse_press() {
    mouse_pressed = 1;
}

void gui_handle_mouse_release() {
    mouse_pressed = 0;
}

/**
 * Check whether a position is valid
 * @param term_x
 * @param term_y
 * @return
 */
int is_valid_position(int term_x, int term_y) {

}

/**
 * Handle mouse move event
 * @param delta_x
 * @param delta_y
 * @return 0 if the movement is valid. For example, when user try to drag a window out of screen, it's invalid
 */
int gui_handle_mouse_move(int delta_x, int delta_y) {
    if (!mouse_pressed) return 0;
}