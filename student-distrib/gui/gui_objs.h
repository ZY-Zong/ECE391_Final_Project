//
// Created by liuzikai on 12/7/19.
//

#ifndef _GUI_OBJS_H
#define _GUI_OBJS_H

#include "../vga/vga.h"
#include "gui_render.h"

typedef struct gui_object_t gui_object_t;
struct gui_object_t {
    unsigned char* canvas;  // if NULL, object is on the video memory
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    vga_argb transparent_color;
};

#define GUI_FONT_FORECOLOR_ARGB    0xFFFFFFFF
#define GUI_FONT_BACKCOLOR_ARGB    0xFF000000

#define GUI_TRANSPARENT_COLOR    0xFFE96F89

void gui_obj_load();
void draw_start_ui();

gui_object_t gui_get_obj_font(char ch);

extern gui_object_t gui_obj_desktop;


#define WIN_UP_WIDTH    652
#define WIN_UP_HEIGHT   21

#define WIN_DOWN_WIDTH 652
#define WIN_DOWN_HEIGHT 4

#define WIN_LEFT_WIDTH 6
#define WIN_LEFT_HEIGHT 480

#define WIN_RIGHT_WIDTH 6
#define WIN_RIGHT_HEIGHT 480

#define WIN_RED_B_WIDTH 11
#define WIN_RED_B_HEIGHT 12

#define WIN_YELLOW_B_WIDTH 12
#define WIN_YELLOW_B_HEIGHT 12

#define WIN_GREEN_B_WIDTH 11
#define WIN_GREEN_B_HEIGHT 12

#define WIN_GREY_B_WIDTH 12
#define WIN_GREY_B_HEIGHT 12


#if GUI_WINDOW_PNG_RENDER
extern vga_argb gui_win_up[WIN_UP_WIDTH * WIN_UP_HEIGHT];
extern vga_argb gui_win_down[WIN_DOWN_WIDTH * WIN_DOWN_HEIGHT];
extern vga_argb gui_win_left[WIN_LEFT_WIDTH * WIN_LEFT_HEIGHT];
extern vga_argb gui_win_right[WIN_RIGHT_WIDTH * WIN_RIGHT_HEIGHT];
extern vga_argb gui_win_red[2][WIN_RED_B_WIDTH * WIN_RED_B_HEIGHT];
extern vga_argb gui_win_yellow[2][WIN_YELLOW_B_WIDTH * WIN_YELLOW_B_HEIGHT];
extern vga_argb gui_win_green[2][WIN_GREEN_B_WIDTH * WIN_GREEN_B_HEIGHT];
extern vga_argb gui_win_grey[WIN_GREY_B_WIDTH * WIN_GREY_B_WIDTH];
#else
extern gui_object_t gui_obj_win_up;
extern gui_object_t gui_obj_win_down;
extern gui_object_t gui_obj_win_left;
extern gui_object_t gui_obj_win_right;
extern gui_object_t gui_obj_red[2];
extern gui_object_t gui_obj_yellow[2];
extern gui_object_t gui_obj_green[2];
extern gui_object_t gui_obj_grey;
#endif

#endif //_GUI_OBJS_H
