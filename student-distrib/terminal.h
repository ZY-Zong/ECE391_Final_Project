//
// Created by qig2 on 10/26/2019.
//

#ifndef TERMINAL_H
#define TERMINAL_H

#include "lib.h"

int32_t system_terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t system_terminal_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t system_terminal_open(const uint8_t* filename);
int32_t system_terminal_close(int32_t fd);

#define KEYBOARD_IRQ_NUM   1
#define KEYBOARD_BUF_SIZE  128


typedef struct terminal_t terminal_t;
struct terminal_t {
    uint8_t valid;
    uint8_t terminal_id;  // equal to slot index

    char key_buf[KEYBOARD_BUF_SIZE];
    uint8_t key_buf_cnt;
    uint8_t user_ask_len;

    int32_t screen_width;
    int32_t screen_height;
    int32_t screen_x;
    int32_t screen_y;
};

#define TERMINAL_MAX_COUNT    3

void terminal_init();
terminal_t* terminal_allocate();
void terminal_deallocate(terminal_t* terminal);

#endif //TERMINAL_H
