//
// Created by qig2 on 10/26/2019.
//
#include "terminal.h"

#define KEYBOARD_PORT   0x60    /* keyboard scancode port */
#define KEYBOARD_BUF_SIZE 128
#define KEYBOARD_FLAG_SIZE 128
#define CTRL_PRESS 0x1D
#define CTRL_RELEASE 0x9D
#define LEFT_SHIFT_PRESS 0x2A
#define LEFT_SHIFT_RELEASE 0xAA
#define RIGHT_SHIFT_PRESS 0x36
#define RIGHT_SHIFT_RELEASE 0xB6
#define BACKSPACE_SCAN_CODE 0x0E

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
//static uint8_t backspace_counter;

/*
 * keyboard_interrupt
 *   REFERENCE: https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Sets.2C_Scan_Codes_and_Key_Codes
 *   DESCRIPTION: Handle the keyboard interrupts
 *   INPUTS: scan code from port 0x60
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: interrupt if necessary
 *   TODO: Change the test case to support new interfaces of handle_scan_code().
 */
void keyboard_interrupt_handler() {

    cli();
    {
        // Get scan code from port 0x60
        uint8_t scancode = inb(KEYBOARD_PORT);

        if (scancode == KEYBOARD_F1_SCANCODE) {  // F1
            clear();
        } else if (scancode < KEYBOARD_SCANCODE_PRESSED) {  // key press
            if (scan_code_table[scancode] != 0) {  // printable character
                handle_scan_code(scancode);  // output the char to the console
            }
        }
    }
    sti();
}

/*
 * handle_scan_code
 * Description: This function handles scancode from the keyboard
 *              and stores the current character into the keyboard buffer.
 * Input: scan_code - scan code from the keyboard
 * Output: None.
 * Side Effect: Modify the keyboard buffer.
 * Assumption: If the keyboard buffer is full, just discard any keystroke from the keyboard.
 */
void handle_scan_code(uint8_t scan_code) {

    if (scan_code >= KEYBOARD_SCANCODE_PRESSED) {
        // If the scan_code is a release code, just reset the flags
        key_flags[scan_code - KEYBOARD_SCANCODE_PRESSED] = 0;
    } else {
        // If the scan_code is a press code, set the flags
        key_flags[scan_code] = 1;
        // If Ctrl+L
        if (1 == key_flags[CTRL_PRESS] && 1 == key_flags[0x26]) {
            clear(); // clear the screen
            reset_cursor(); // reset the cursor to up-left corner
            return;
        }
//        // If backspace
//        // Just putc then delete the char in the keyboard_buf
//        if (1 == key_flags[BACKSPACE_SCAN_CODE]) {
//            if (backspace_counter < (keyboard_buf_counter / 2)) {
//                keyboard_buf[keyboard_buf_counter] = '\b';
//                putc(keyboard_buf[keyboard_buf_counter]);
//                keyboard_buf_counter++;
//                backspace_counter++;
//            }
//        }
        // If shift is pressed
        if ((1 == key_flags[LEFT_SHIFT_PRESS]) || (1 == key_flags[RIGHT_SHIFT_PRESS])) {
            if (0 != shift_scan_code_table[scan_code]) {
                if (keyboard_buf_counter < KEYBOARD_BUF_SIZE) {
                    keyboard_buf[keyboard_buf_counter] = shift_scan_code_table[scan_code];
                    putc(keyboard_buf[keyboard_buf_counter]);
                    keyboard_buf_counter++;
                }
            }
        } else {
            // If shift is not pressed
            if (0 != scan_code_table[scan_code]) {
                if (keyboard_buf_counter < KEYBOARD_BUF_SIZE) {
                    keyboard_buf[keyboard_buf_counter] = scan_code_table[scan_code];
                    putc(keyboard_buf[keyboard_buf_counter]);
                    keyboard_buf_counter++;
                }
            }
        }
    }
}

/*
 * keyboard_init
 * Description: This function is used to initialize keyboard.
 *              Set key_flags to 0s, and clean up the keyboard_buf.
 * Input: None.
 * Output: None.
 * Side Effect: Modify keyboard's static variables.
 */
void keyboard_init() {
    int i;
    // Initialize the keyboard flags
    for (i = 0; i < KEYBOARD_FLAG_SIZE; i++) {
        key_flags[i] = 0;
    }
    // Initialize the keyboard buffer
    for (i = 0; i < KEYBOARD_BUF_SIZE; i++) {
        keyboard_buf[i] = 0;
    }
    keyboard_buf_counter = 0;
}

/*
 * terminal_open
 * Description: This function initializes key_flag and keyboard_buf.
 * Input: None.
 * Output: 0 (Success).
 * Side Effect: Modify terminal's static variables.
 */
int32_t terminal_open(const uint8_t *filename) {
    return 0;
}

/*
 * terminal_close
 * Description: This function resets key_flag and keyboard_buf.
 * Input: None.
 * Output: 0 (Success).
 * Side Effect: Modify terminal's static variables.
 */
int32_t terminal_close(int32_t fd) {
    return 0;
}

/*
 * terminal_read
 * Description: This function copies keyboard buffer to user buffer.
 *              It also resets the keyboard buffer.
 * Input: None.
 * Output: Returns the number of bytes read.
 * Side Effect: Modify both keyboard's static variables.
 */
int32_t terminal_read(int32_t fd, void *buf, int32_t nbytes) {
    int32_t i = 0;  // record how many characters are read from the keyboard buffer before we reach count, keyboard_buf_size or '\n'
    int32_t to_delete = 0;
    int32_t j;  // counter
    //int32_t delete;
    //int32_t delete_backspace;

    // Critical section to prevent keyboard buffer changes during the copy operation.
    while (i < nbytes && to_delete < KEYBOARD_BUF_SIZE) {
        cli();
        {
            if (to_delete < keyboard_buf_counter) {
                // If we see an enter
                if (keyboard_buf[to_delete] == '\n') {
                    sti();
                    break;
                }
                // If we see a backspace
                if (keyboard_buf[to_delete] == '\b') {
                    // If
                    if (0 == i) {
                        to_delete++;
                        continue;
                    }
                    i--;
                    to_delete++;
                    continue;
                }
                ((char *) buf)[i] = keyboard_buf[to_delete];
                i++;
                to_delete++;
            }
        }
        sti();
    }

    cli();
    {
        // Reset the keyboard buffer
        if (to_delete == keyboard_buf_counter) {
            // If we read all the characters in the keyboard buffer, just clean up the keyboard buffer
            keyboard_buf_counter = 0; // cleans up the keyboard buffer
        } else {
            // If we just read part of the buffer, we need to retain the rest of unread characters.
            to_delete += (keyboard_buf[to_delete] == '\n');
//            delete = to_delete + (keyboard_buf[to_delete] == '\n');
            for (j = to_delete; j < keyboard_buf_counter; j++) {
                keyboard_buf[j - to_delete] = keyboard_buf[j];
            }
            keyboard_buf_counter -= to_delete;
//            for (j = delete; j < keyboard_buf_counter; j++) {
//                keyboard_buf[j - delete] = keyboard_buf[j];
//            }
//            keyboard_buf_counter -= delete;
        }
    }
    sti();

    return i;
}

/*
 * terminal_write
 * Description: This function print the user buffer to the screen.
 * Input: None.
 * Output: Returns the number of bytes printed.
 * Side Effect: None.
 */
int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes) {
    int i;
    // Critical section to prevent keyboard buffer changes during the copy operation.
    cli();
    {
        for (i = 0; i < nbytes; i++) {
            putc(((uint8_t *) buf)[i]);
        }
    }
    sti();
    return i;
}

