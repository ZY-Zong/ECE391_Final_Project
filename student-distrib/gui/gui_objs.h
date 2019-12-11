//
// Created by liuzikai on 12/7/19.
//

#ifndef _GUI_OBJS_H
#define _GUI_OBJS_H

#include "../vga/vga.h"

typedef struct gui_object_t gui_object_t;
struct gui_object_t {
    unsigned char* canvas;  // if NULL, object is on the video memory
    int x;
    int y;
    unsigned int width;
    unsigned int height;
};

#define GUI_FONT_FORECOLOR_ARGB    0xFFFFFFFF
#define GUI_FONT_BACKCOLOR_ARGB    0xFF000000

void gui_obj_load();

gui_object_t gui_get_obj_font(char ch);
extern gui_object_t gui_obj_desktop;
extern gui_object_t gui_obj_win_up;
extern gui_object_t gui_obj_win_down;
extern gui_object_t gui_obj_win_left;
extern gui_object_t gui_obj_win_right;
extern gui_object_t gui_obj_red[2];
extern gui_object_t gui_obj_yellow[2];
extern gui_object_t gui_obj_green[2];

#endif //_GUI_OBJS_H
