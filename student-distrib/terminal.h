//
// Created by qig2 on 10/26/2019.
//

#ifndef TERMINAL_H
#define TERMINAL_H

#include "lib.h"

#define KEYBOARD_IRQ_NUM   1
#define KEYBOARD_BUF_SIZE 128

// TODO: revise these functions to support virtual terminal
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);

// Declaration of keyboard related functions
void keyboard_init();
void keyboard_interrupt_handler();
void handle_scan_code(uint8_t scan_code);

#define KEYBOARD_F1_SCANCODE 0x3B
#define KEYBOARD_F2_SCANCODE 0x3C
#define KEYBOARD_F3_SCANCODE 0x3D
#define KEYBOARD_SCANCODE_PRESSED 0x80

typedef struct terminal_control_t terminal_control_t;
struct terminal_control_t {
    // Bit vector used to check whether someone is reading from the keyboard
    uint8_t whether_read;
    // Keyboard buffer of size 128 and a counter to store the current position in the buffer
    char keyboard_buf[KEYBOARD_BUF_SIZE];
    uint8_t keyboard_buf_counter;
};

// TODO: implement this function
void terminal_control_init(terminal_control_t* terminal_control);

#endif //TERMINAL_H
