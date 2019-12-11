//
// Created by liuzikai on 12/7/19.
//

#include "gui.h"

int gui_inited = 0;

static gui_window_t test_win;

void gui_init() {
    gui_obj_load();
    gui_window_init();
    gui_new_window(&test_win, (char *) screen_char);  // only for test
    gui_inited = 1;
}