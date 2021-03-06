//
// Created by liuzikai on 12/7/19.
//

#ifndef _GUI_RENDER_H
#define _GUI_RENDER_H

// If enabled, window components are not pre-rendered into GUI objects, but only pre-loaded as png data,
// which allow alpha blending, but it's proven to be too slow on QEMU
#define GUI_WINDOW_PNG_RENDER    0

void gui_render();

#define WIN_UP_BORDER_LEFT_MARGIN       6
#define WIN_UP_BORDER_UP_MARGIN         21
#define WIN_LEFT_BORDER_LEFT_MARGIN     6
#define WIN_DOWN_BORDER_LEFT_MARGIN     6
#define WIN_DOWN_BORDER_UP_MARGIN       TERMINAL_HEIGHT_PIXEL
#define WIN_RIGHT_BORDER_LEFT_MARGIN    TERMINAL_WIDTH_PIXEL
#define WIN_RED_B_LEFT_MARGIN           4
#define WIN_RED_B_UP_MARGIN             17
#define WIN_YELLOW_B_LEFT_MARGIN        22
#define WIN_YELLOW_B_UP_MARGIN          17
#define WIN_GREEN_B_LEFT_MARGIN         41
#define WIN_GREEN_B_UP_MARGIN           17

#define CLOCK_START_X    (VGA_WIDTH - FONT_WIDTH * 8 - 5)
#define CLOCK_START_Y    (3)

#define STATUS_BAR_HEIGHT    23

#define TERMINAL_B_X    (CLOCK_START_X - WIN_TERMINAL_B_WIDTH - 10)
#define TERMINAL_B_Y    (1)

extern int gui_term_button_pressed;

#endif //_GUI_RENDER_H
