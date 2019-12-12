//
// Created by liuzikai on 12/7/19.
//

#include "gui.h"
#include "gui_objs.h"

int gui_inited = 0;

void gui_init() {
    gui_obj_load();
    gui_window_init();
    gui_inited = 1;
}