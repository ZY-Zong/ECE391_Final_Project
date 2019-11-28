//
// Created by qig2 on 10/26/2019.
//

#ifndef TERMINAL_H
#define TERMINAL_H

#include "lib.h"

// TODO: revise these functions to support virtual terminal
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);

#define KEYBOARD_IRQ_NUM   1
#define KEYBOARD_BUF_SIZE 128

// Declaration of keyboard related functions
//void keyboard_init();
void keyboard_interrupt_handler();
void handle_scan_code(uint8_t scan_code);

#define KEYBOARD_F1_SCANCODE 0x3B
#define KEYBOARD_F2_SCANCODE 0x3C
#define KEYBOARD_F3_SCANCODE 0x3D
#define KEYBOARD_SCANCODE_PRESSED 0x80

typedef struct terminal_t terminal_t;
struct terminal_t {
    uint8_t valid;

    char keyboard_buf[KEYBOARD_BUF_SIZE];
    uint8_t keyboard_buf_counter;
    uint8_t user_ask_len;

    int32_t screen_width;
    int32_t screen_height;
    int32_t screen_x;
    int32_t screen_y;
    char* vidmem;
};

#define MAX_TERMINAL_COUNT    3

void terminal_init();
terminal_t* terminal_allocate();
void terminal_deallocate(terminal_t* terminal);

#endif //TERMINAL_H
