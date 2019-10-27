//
// Created by qig2 on 10/26/2019.
//

#ifndef TERMINAL_H
#define TERMINAL_H
#include "linkage.h"
/** Keyboard related constants */
#define KEYBOARD_IRQ_NUM   1
#define KEYBOARD_PORT   0x60    /* keyboard scancode port */
#define KEYBOARD_BUF_SIZE 128
#define KEYBOARD_FLAG_SIZE 128
#define CTRL_PRESS 0x1D
#define CTRL_RELEASE 0x9D
#define LEFT_SHIFT_PRESS 0x2A
#define LEFT_SHIFT_RELEASE 0xAA
#define RIGHT_SHIFT_PRESS 0x36
#define RIGHT_SHIFT_RELEASE 0xB6

/* Keys that correspond to scan codes, using scan code set 1 for "US QWERTY" keyboard
 * REFERENCE: https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Sets.2C_Scan_Codes_and_Key_Codes
 * TODO: Not handled keys: Esc, Tab, right ctrl, Caps.
 */
static const char scan_code_table[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',      /* 0x00 - 0x0E */
        0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',      /* 0x0F - 0x1C */
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',           /* 0x1D - 0x29 */
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0,          /* 0x2A - 0x37 */
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            /* 0x38 - 0x46 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    /* 0x47 - 0x53 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        /* 0x54 - 0x80 */
};
static const char shift_scan_code_table[128] = {
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',      /* 0x00 - 0x0E */
        0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',      /* 0x0F - 0x1C */
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',           /* 0x1D - 0x29 */
        0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0,          /* 0x2A - 0x37 */
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            /* 0x38 - 0x46 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    /* 0x47 - 0x53 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        /* 0x54 - 0x80 */
};

// Array to record what keys has been pressed currently
static uint8_t key_flags[KEYBOARD_FLAG_SIZE];

// Keyboard buffer of size 128 and a counter to store the current position in the buffer
static char keyboard_buf[KEYBOARD_BUF_SIZE];
static uint8_t keyboard_buf_counter;


// System call interfaces for terminal
int terminal_open(const char __user *filename, int flags, int mode);
int terminal_close(unsigned int fd);
int terminal_read(unsigned int fd, char __user *buf, size_t count);
int terminal_write(unsigned int fd, const char __user *buf, size_t count);

// Declaration of keyboard related functions
void keyboard_init();
void keyboard_interrupt_handler();
void handle_scan_code(uint8_t scan_code);

#define KEYBOARD_F1_SCANCODE 0x3B
#define KEYBOARD_F2_SCANCODE 0x3C
#define KEYBOARD_F3_SCANCODE 0x3D
#define KEYBOARD_SCANCODE_PRESSED 0x80

#endif //TERMINAL_H
