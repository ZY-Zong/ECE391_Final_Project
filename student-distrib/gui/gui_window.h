//
// Created by liuzikai on 12/10/19.
//

#ifndef _GUI_WINDOW_H
#define _GUI_WINDOW_H

#define GUI_WIN_ENABLE_LOG    1

typedef struct gui_window_t gui_window_t;
struct gui_window_t {
    int term_x;
    int term_y;
    char* screen_char;
    int terminal_id;
};

#define GUI_MAX_WINDOW_NUM    5

#define GUI_WIN_INITIAL_TERM_X  50
#define GUI_WIN_INITIAL_TERM_Y  50
#define GUI_WIN_INITIAL_OFFSET_X  50
#define GUI_WIN_INITIAL_OFFSET_Y  50
#define GUI_WIN_LEFT_RIGHT_MARGIN    6
#define GUI_WIN_TITLE_BAR_HEIGHT    21
#define GUI_WIN_DOWN_MARGIN    4


extern gui_window_t* window_stack[GUI_MAX_WINDOW_NUM];  // 0 window is on the top

void gui_window_init();

int gui_new_window(gui_window_t *win, char *screen_buf, int terminal_id);
int gui_activate_window(gui_window_t* win);
int gui_destroy_window(gui_window_t* win);

void gui_handle_mouse_press(int x, int y);
int gui_handle_mouse_move(int delta_x, int delta_y);
void gui_handle_mouse_release(int x, int y);

typedef enum cursor_position_t cursor_position_t;
enum cursor_position_t {
    NOT_IN_WINDOW,
    IN_BODY,
    IN_TITLE_BAR,
    ON_BORDER,
    IN_RED_BUTTON,
    IN_YELLOW_BUTTON,
    IN_GREEN_BUTTON
};

int check_inside_window(int x, int y, const gui_window_t *win);

#endif //_GUI_WINDOW_H
