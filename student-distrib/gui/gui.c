//
// Created by liuzikai on 12/7/19.
//

#include "gui.h"
#include "gui_objs.h"

int gui_inited = 0;

static gui_window_t test_win;
static gui_window_t test_win2;
static gui_window_t test_win3;

void gui_init() {
    gui_obj_load();
    gui_window_init();
    gui_new_window(&test_win, (char *) screen_char);  // only for test
    gui_new_window(&test_win2, (char *) screen_char);  // only for test
    gui_new_window(&test_win3, (char *) screen_char);  // only for test
    gui_inited = 1;
}