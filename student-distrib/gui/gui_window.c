//
// Created by liuzikai on 12/10/19.
//

// TODO: apply locks carefully

#include "gui_window.h"

#include "../lib.h"
#include "../vga/vga.h"
#include "gui_render.h"
#include "gui_objs.h"
#include "../task/task.h"

gui_window_t *window_stack[GUI_MAX_WINDOW_NUM];
static int mouse_pressed_on_title = 0;

/**
 * Initialize GUI window control
 */
void gui_window_init() {
    int i;
    for (i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
        window_stack[i] = NULL;
    }
}

/**
 * Find a window in window stack
 * @param win    The window to find, or NULL
 * @return Its index in window_stack, or -1 if it's not in the stack
 */
static int find_window_idx(const gui_window_t *win) {
    int i;
    for (i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
        if (window_stack[i] == win) return i;
    }
    return -1;
}

/**
 * Initialize a new window
 * @param win           The window to be initialized
 * @param screen_buf    Character buffer (on screen content) of the terminal of the window
 * @param terminal_id   ID of the terminal of the window
 * @return 0 for success, -1 for failure
 */
int gui_new_window(gui_window_t *win, char *screen_buf, int terminal_id) {

    if (find_window_idx(win) != -1) {
        DEBUG_ERR("gui_new_window(): window already exists");
        return -1;
    }

    int idx = find_window_idx(NULL);

    if (idx == -1) {
        DEBUG_ERR("gui_new_window(): reach maximal window");
        return -1;
    }

    win->term_x = GUI_WIN_INITIAL_TERM_X + GUI_WIN_INITIAL_OFFSET_X * idx;
    win->term_y = GUI_WIN_INITIAL_TERM_Y + GUI_WIN_INITIAL_OFFSET_Y * idx;
    win->screen_char = screen_buf;
    win->terminal_id = terminal_id;

    window_stack[idx] = win;

    return 0;
}

/**
 * Activate a window
 * @param win    The window to be activated
 * @return 0 on success, -1 on failure
 */
int gui_activate_window(gui_window_t *win) {
    int idx = find_window_idx(win);
    if (idx == -1) {
        DEBUG_ERR("gui_activate_window(): no such window");
        return -1;
    }

    if (window_stack[0] == win) return 0;

#if GUI_WIN_ENABLE_LOG
    DEBUG_PRINT("GUI window %d gets activated", idx);
#endif

    // Put the window to the top of the stack
    int i;
    for (i = idx; i >= 1; i--) {
        window_stack[i] = window_stack[i - 1];
    }
    window_stack[0] = win;

    task_change_focus(win->terminal_id);

    return 0;
}

/**
 * Destroy a window
 * @param win    The window to be destroyed
 * @return 0 on success, -1 on failure
 */
int gui_destroy_window(gui_window_t *win) {
    int idx = find_window_idx(win);
    if (idx == -1) {
        DEBUG_ERR("gui_destroy_window(): no such window");
        return -1;
    }

    // Move all windows forward
    int i;
    for (i = idx; i < GUI_MAX_WINDOW_NUM - 1; i++) {
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
 * @return One of cursor_position_t
 */
int check_inside_window(int x, int y, const gui_window_t *win) {
    if (win == NULL) return NOT_IN_WINDOW;
    int term_x = win->term_x;
    int term_y = win->term_y;
    if (x < term_x - GUI_WIN_LEFT_RIGHT_MARGIN || x >= term_x + TERMINAL_WIDTH_PIXEL + GUI_WIN_LEFT_RIGHT_MARGIN ||
        y < term_y - GUI_WIN_TITLE_BAR_HEIGHT || y >= term_y + TERMINAL_HEIGHT_PIXEL + GUI_WIN_DOWN_MARGIN) {
        return NOT_IN_WINDOW;
    }
    if (y < term_y) {
        if (x >= term_x + WIN_RED_B_LEFT_MARGIN && x < term_x + WIN_RED_B_LEFT_MARGIN + WIN_RED_B_WIDTH && 
            y >= term_y - WIN_RED_B_UP_MARGIN && y < term_y - WIN_RED_B_UP_MARGIN + WIN_RED_B_HEIGHT) {
            return IN_RED_BUTTON;
        }
        if (x >= term_x + WIN_YELLOW_B_LEFT_MARGIN && x < term_x + WIN_YELLOW_B_LEFT_MARGIN + WIN_YELLOW_B_WIDTH &&
            y >= term_y - WIN_YELLOW_B_UP_MARGIN && y < term_y - WIN_YELLOW_B_UP_MARGIN + WIN_YELLOW_B_HEIGHT) {
            return IN_YELLOW_BUTTON;
        }
        if (x >= term_x + WIN_GREEN_B_LEFT_MARGIN && x < term_x + WIN_GREEN_B_LEFT_MARGIN + WIN_GREEN_B_WIDTH &&
            y >= term_y - WIN_GREEN_B_UP_MARGIN && y < term_y - WIN_GREEN_B_UP_MARGIN + WIN_GREEN_B_HEIGHT) {
            return IN_GREEN_BUTTON;
        }
        return IN_TITLE_BAR;
    }
    if (x >= term_x && x < term_x + TERMINAL_WIDTH_PIXEL &&
        y >= term_y && y < term_y + TERMINAL_HEIGHT_PIXEL) {
        return IN_BODY;
    }
    return ON_BORDER;
}

/**
 * Handle mouse press event
 * @param x    X coordinate of cursor when mouse is pressed
 * @param y    Y coordinate of cursor when mouse is pressed
 */
void gui_handle_mouse_press(int x, int y) {

    int i = 0;
    cursor_position_t state = NOT_IN_WINDOW;
    for (i = 0; i < GUI_MAX_WINDOW_NUM; i++) {
        if ((state = check_inside_window(x, y, window_stack[i])) != NOT_IN_WINDOW) break;
    }

    if (i == GUI_MAX_WINDOW_NUM) {  // not in any window
        if (y < STATUS_BAR_HEIGHT && x >= TERMINAL_B_X && x < TERMINAL_B_X + WIN_TERMINAL_B_WIDTH) {
            gui_term_button_pressed = 1;
        }
        return;
    }

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

/**
 * Handle mouse release event
 * @param x    X coordinate of cursor when mouse is released
 * @param y    Y coordinate of cursor when mouse is released
 */
void gui_handle_mouse_release(int x, int y) {
    mouse_pressed_on_title = 0;
    if (check_inside_window(x, y, window_stack[0]) == IN_RED_BUTTON) {
#if GUI_WIN_ENABLE_LOG
        DEBUG_PRINT("GUI window close");
#endif
        task_halt_terminal(window_stack[0]->terminal_id);
        return;
    }

    gui_term_button_pressed = 0;  // always
    if (y < STATUS_BAR_HEIGHT && x >= TERMINAL_B_X && x < TERMINAL_B_X + WIN_TERMINAL_B_WIDTH) {
        system_execute((uint8_t *) "shell", 0, 1, NULL);
        return;
    }
}

/**
 * Check whether a window position is valid
 * @param term_x    X coordinate of upper-left corner of terminal (window body, not window border)
 * @param term_y    Y coordinate of upper-left corner of terminal (window body, not window border)
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

    gui_window_t *win = window_stack[0];

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
