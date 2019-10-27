//
// Created by qig2 on 10/26/2019.
//

#ifndef TERMINAL_H
#define TERMINAL_H

#include "lib.h"

#define KEYBOARD_IRQ_NUM   1

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

#endif //TERMINAL_H
