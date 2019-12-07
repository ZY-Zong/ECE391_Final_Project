//
// Created by liuzikai on 12/7/19.
//

#include "gui.h"

void gui_print_char(char ch, int x, int y) {
    gui_object_t c = gui_obj_font(ch);
    vga_buf_copy(c.src_addr, x, y, c.width, c.height);
}