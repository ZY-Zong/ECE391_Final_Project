//
// Created by qig2 on 10/26/2019.
//
#include "lib.h"
#include "terminal.h"
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

        // TODO: eliminate these function for demo of checkpoint 1
        if (scancode == KEYBOARD_F1_SCANCODE) {  // F1
            clear();
#ifdef RUN_TESTS
            } else if (scancode == KEYBOARD_F2_SCANCODE) {  // F2
            divide_zero_test();
        } else if (scancode == KEYBOARD_F3_SCANCODE) {  // F3
            dereference_null_test();
#endif
        } else if (scancode < KEYBOARD_SCANCODE_PRESSED) {  // key press
            if (scan_code_table[scancode] != 0) {  // printable character
                handle_scan_code(scancode);  // output the char to the console
            }
#ifdef RUN_TESTS
            test1_handle_typing(scan_code_table[scancode]);
#endif
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
        // If shift is pressed
        if ((1 == key_flags[LEFT_SHIFT_PRESS]) || (1 == key_flags[RIGHT_SHIFT_PRESS])) {
            if (0 != shift_scan_code_table[scan_code]) {
                if (keyboard_buf_counter < KEYBOARD_BUF_SIZE - 1) {
                    keyboard_buf[keyboard_buf_counter] = shift_scan_code_table[scan_code];
                    keyboard_buf_counter++;
                }
            }
        } else {
            // If shift is not pressed
            if (0 != scan_code_table[scan_code]) {
                if (keyboard_buf_counter < KEYBOARD_BUF_SIZE - 1) {
                    keyboard_buf[keyboard_buf_counter] = scan_code_table[scan_code];
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
int terminal_open(const char __user *filename, int flags, int mode) {
    return 0;
}

/*
 * terminal_close
 * Description: This function resets key_flag and keyboard_buf.
 * Input: None.
 * Output: 0 (Success).
 * Side Effect: Modify terminal's static variables.
 */
int terminal_close(unsigned int fd) {
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
int terminal_read(unsigned int fd, char __user *buf, size_t count) {
    int i; // record how many characters are read from the keyboard buffer before we reach count, keyboard_buf_size or '\n'
    int min; // the minimum of count and keyboard_buf_size
    int j; // counter
    // Critical section to prevent keyboard buffer changes during the copy operation.
    cli();
    {
        // Calculate the number of bytes needed to copy from keyboard to the user buffer.
        // Which is the minimum of count and keyboard_buf_size.
        if (count >= keyboard_buf_counter) {
            min = keyboard_buf_counter;
        } else {
            min = count;
        }
        // Copy the number of bytes required from keyboard to the user buffer
        for (i = 0; (i < min) && ('\n' != keyboard_buf[i]); i++) {
            buf[i] = keyboard_buf[i];
        }
        // Reset the keyboard buffer
        if (i == keyboard_buf_counter) {
            // If we read all the characters in the keyboard buffer, just clean up the keyboard buffer
            keyboard_buf_counter = 0; // cleans up the keyboard buffer
        } else {
            // If we just read part of the buffer, we need to retain the rest of unread characters.
            for (j = i; j < keyboard_buf_counter; j++) {
                keyboard_buf[j - i] = keyboard_buf[j];
            }
            keyboard_buf_counter -= i;
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
int terminal_write(unsigned int fd, const char __user *buf, size_t count) {
    int i;
    // Critical section to prevent keyboard buffer changes during the copy operation.
    cli();
    {
        for (i = 0; i < count; i++) {
            putc(buf[i]);
        }
    }
    sti();
    return i;
}

