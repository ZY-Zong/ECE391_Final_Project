//
// Created by liuzikai on 12/7/19.
//

#ifndef _GUI_OBJS_H
#define _GUI_OBJS_H

#include "../vga/vga.h"

typedef struct gui_object_t gui_object_t;
struct gui_object_t {
    char* src_addr;
    int width;
    int height;
};

#define GUI_FONT_FORECOLOR    0xFFFFFF
#define GUI_FONT_BACKCOLOR    0x000000

void gui_obj_load();

gui_object_t gui_obj_font(char ch);

#endif //_GUI_OBJS_H
