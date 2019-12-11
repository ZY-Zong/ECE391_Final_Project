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

#define WIN_UP_WIDTH    652
#define WIN_UP_HEIGHT   21
extern vga_argb gui_win_up[WIN_UP_WIDTH * WIN_UP_HEIGHT];

#define WIN_DOWN_WIDTH 652
#define WIN_DOWN_HEIGHT 4
extern vga_argb gui_win_down[WIN_DOWN_WIDTH * WIN_DOWN_HEIGHT];

#define WIN_LEFT_WIDTH 6
#define WIN_LEFT_HEIGHT 480
extern vga_argb gui_win_left[WIN_LEFT_WIDTH * WIN_LEFT_HEIGHT];

#define WIN_RIGHT_WIDTH 6
#define WIN_RIGHT_HEIGHT 480
extern vga_argb gui_win_right[WIN_RIGHT_WIDTH * WIN_RIGHT_HEIGHT];

#define WIN_RED_B_WIDTH 11
#define WIN_RED_B_HEIGHT 12
extern vga_argb gui_win_red[2][WIN_RED_B_WIDTH * WIN_RED_B_HEIGHT];

#define WIN_YELLOW_B_WIDTH 12
#define WIN_YELLOW_B_HEIGHT 12
extern vga_argb gui_win_yellow[2][WIN_YELLOW_B_WIDTH * WIN_YELLOW_B_HEIGHT];

#define WIN_GREEN_B_WIDTH 11
#define WIN_GREEN_B_HEIGHT 12
extern vga_argb gui_win_green[2][WIN_GREEN_B_WIDTH * WIN_GREEN_B_HEIGHT];

#endif //_GUI_OBJS_H
