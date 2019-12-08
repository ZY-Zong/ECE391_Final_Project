//
// Created by liuzikai on 12/7/19.
//

#ifndef _GUI_RENDER_H
#define _GUI_RENDER_H

typedef struct gui_component_t gui_component_t;
struct gui_component_t {
    int type;
    unsigned int flag;
    int (*render) (int x, int y);  // render function
};

#endif //_GUI_RENDER_H
