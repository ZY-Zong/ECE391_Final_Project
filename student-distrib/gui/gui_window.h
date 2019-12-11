//
// Created by liuzikai on 12/10/19.
//

#ifndef _GUI_WINDOW_H
#define _GUI_WINDOW_H

typedef struct gui_window_t gui_window_t;
struct gui_window_t {
    int term_x;
    int term_y;
    char* screen_char;
};

#define GUI_MAX_WINDOW_NUM    3

#define TERMINAL_WIDTH_PIXEL     640
#define TERMINAL_HEIGHT_PIXEL    480

#define GUI_WIN_INITIAL_TERM_X  50
#define GUI_WIN_INITIAL_TERM_Y  50

extern gui_window_t* window_stack[GUI_MAX_WINDOW_NUM];  // 0 window is on the top

void gui_window_init();

int gui_new_window(gui_window_t *win, char *screen_buf);
int gui_activate_window(gui_window_t* win);
int gui_destroy_window(gui_window_t* win);

void gui_handle_mouse_press();
int gui_handle_mouse_move(int delta_x, int delta_y);
void gui_handle_mouse_release();

#endif //_GUI_WINDOW_H
