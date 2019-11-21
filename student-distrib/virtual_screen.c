//
// Created by liuzikai on 11/12/19.
//

#include "task.h"
#include "virtual_screen.h"
/**
 * Initialize the virtual_screen_t instance used for multi terminal.
 * @param virtual_screen - The pointer to the virtual_screen_t field in task_t
 */
 // TODO: Initialize the video_mem field
void virtual_screen_init(virtual_screen_t* virtual_screen) {
    virtual_screen -> screen_width = TEXT_MODE_WIDTH;
    virtual_screen -> screen_height = TEXT_MODE_HEIGHT;
    virtual_screen -> screen_x = 0;
    virtual_screen -> screen_y = 0;
}
