//
// Created by Zhenyu Zong on 2019/12/9.
//

#ifndef _WINDOW_H
#define _WINDOW_H

#include "../gui/upng.h"
#include "../vga/vga.h"
#include "../file_system.h"

#define TERMINAL_X 50
#define TERMINAL_Y 60

void full_screen_png();
void draw_terminal_status_bar();
void draw_pressed_yellow_button();
void draw_pressed_green_button();


#endif //_WINDOW_H
