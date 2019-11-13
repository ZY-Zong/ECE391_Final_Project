//
// Created by liuzikai on 11/12/19.
//

#ifndef _VIRTUAL_SCREEN_H
#define _VIRTUAL_SCREEN_H

typedef struct virtual_screen_t virtual_screen_t;
struct virtual_screen_t {
    int screen_width;
    int screen_height;
    int screen_x;
    int screen_y;
    char* video_mem;
};

// TODO: implement this function
void virtual_screen_init(virtual_screen_t* virtual_screen)

#endif // _VIRTUAL_SCREEN_H
