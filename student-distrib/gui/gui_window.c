//
// Created by liuzikai on 12/10/19.
//

// TODO: apply locks carefully

#include "gui_window.h"

#include "../lib.h"
#include "../vga/vga.h"

gui_window_t *window_stack[GUI_MAX_WINDOW_NUM];
static int mouse_pressed_on_title = 0;

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

#if GUI_WIN_ENABLE_LOG
    DEBUG_PRINT("GUI window %d gets activated", idx);
#endif

    // Put the window to the top of the stack
    for (int i = idx; i >= 1; i--) {
        window_stack[i] = window_stack[i - 1];
    }
    window_stack[0] = win;

    return 0;
}

int gui_destroy_window(gui_window_t *win) {
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

typedef enum cursor_position_t cursor_position_t;
enum cursor_position_t {
    NOT_IN_WINDOW,
    IN_BODY,
    IN_TITLE_BAR
};

/**
 * Check whether the cursor is inside a window
 * @param x
 * @param y
 * @param win
 * @return One of cursor_position_t
 */
int check_inside_window(int x, int y, const gui_window_t *win) {
    if (win == NULL) return NOT_IN_WINDOW;
    int term_x = win->term_x;
    int term_y = win->term_y;
    if (x < term_x - GUI_WIN_LEFT_RIGHT_MARGIN || x > term_x + TERMINAL_WIDTH_PIXEL + GUI_WIN_LEFT_RIGHT_MARGIN ||
        y < term_y - GUI_WIN_TITLE_BAR_HEIGHT || y > term_y + TERMINAL_HEIGHT_PIXEL + GUI_WIN_DOWN_MARGIN) {
        return NOT_IN_WINDOW;
    }
    if (y < term_y) {
        return IN_TITLE_BAR;
    }
    return IN_BODY;
}

void gui_handle_mouse_press(int x, int y) {
    int i = 0;
    cursor_position_t state = NOT_IN_WINDOW;
    for (i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
        if ((state = check_inside_window(x, y, window_stack[i])) != NOT_IN_WINDOW) break;
    }
    if (i == GUI_MAX_WINDOW_NUM) return;  // not in any window
    if (i != 0) {
        gui_activate_window(window_stack[i]);
    }
    i = 0;  // now the window is at the top of the stack
    if (state == IN_TITLE_BAR) {
        mouse_pressed_on_title = 1;
#if GUI_WIN_ENABLE_LOG
        DEBUG_PRINT("GUI window press on title bar");
#endif
    }
#if GUI_WIN_ENABLE_LOG
    DEBUG_PRINT("GUI window press in window body");
#endif
}

void gui_handle_mouse_release(int x, int y) {
    mouse_pressed_on_title = 0;
}

/**
 * Check whether a position is valid
 * @param term_x
 * @param term_y
 * @return 1 if valid, 0 if not
 */
int is_valid_position(int term_x, int term_y) {
    if (term_x - GUI_WIN_LEFT_RIGHT_MARGIN < 0) return 0;
    if (term_x + TERMINAL_WIDTH_PIXEL + GUI_WIN_LEFT_RIGHT_MARGIN > VGA_WIDTH) return 0;
    if (term_y - GUI_WIN_TITLE_BAR_HEIGHT < 0) return 0;
    if (term_y + TERMINAL_HEIGHT_PIXEL + GUI_WIN_DOWN_MARGIN > VGA_HEIGHT) return 0;
    return 1;
}

/**
 * Handle mouse move event
 * @param delta_x
 * @param delta_y
 * @return 0 if the movement is valid. When user try to drag a window out of screen, it's invalid, return -1
 */
int gui_handle_mouse_move(int delta_x, int delta_y) {
    if (!mouse_pressed_on_title) return 0;

    gui_window_t* win = window_stack[0];

    if (win == NULL) {
        DEBUG_ERR("gui_handle_mouse_move(): invalid top window!");
        return 0;
    }

    if (is_valid_position(win->term_x + delta_x, win->term_y + delta_y)) {
        win->term_x += delta_x;
        win->term_y += delta_y;
        return 0;
    }
    return -1;
}