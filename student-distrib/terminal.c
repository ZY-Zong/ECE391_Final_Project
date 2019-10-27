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
                keyboard_buf[keyboard_buf_counter] = shift_scan_code_table[scan_code];
                keyboard_buf_counter++;
            }
        } else {
            // If shift is not pressed
            if (0 != scan_code_table[scan_code]) {
                keyboard_buf[keyboard_buf_counter] = scan_code_table[scan_code];
                keyboard_buf_counter++;
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
int terminal_open() {
    int i;
    // Initialize the terminal buffer
    for (i = 0; i < KEYBOARD_BUF_SIZE; i++) {
        terminal_buf[i] = 0;
    }
    terminal_buf_counter = 0;
    return 0;
}

/*
 * terminal_close
 * Description: This function resets key_flag and keyboard_buf.
 * Input: None.
 * Output: 0 (Success).
 * Side Effect: Modify terminal's static variables.
 */
int terminal_close() {
    // Reset the terminal buffer
    terminal_buf_counter = 0;
    return 0;
}
/*
 * terminal_read
 * Description: This function copies keyboard buffer to terminal buffer.
 *              It also cleans up the keyboard buffer.
 * Input: None.
 * Output: Returns the number of bytes read.
 * Side Effect: Modify both keyboard and terminal's static variables.
 */
int terminal_read() {
    int i;
    // Critical section to prevent keyboard buffer changes during the copy operation.
    cli();
    {
        terminal_buf_counter = keyboard_buf_counter;
        for (i = 0; i < terminal_buf_counter; i++) {
            terminal_buf[i] = keyboard_buf[i];
        }
        keyboard_buf_counter = 0; // cleans up the keyboard buffer
    }
    sti();
    return i;
}
/*
 * terminal_write
 * Description: This function print the terminal buffer to the screen.
 *              It also cleans up the terminal buffer.
 * Input: None.
 * Output: Returns the number of bytes printed.
 * Side Effect: Reset terminal's buffer.
 */
int terminal_write() {
    int i;
    // Critical section to prevent keyboard buffer changes during the copy operation.
    cli();
    {
        for (i = 0; i < terminal_buf_counter; i++) {
            putc(terminal_buf[i]);
        }
        terminal_buf_counter = 0; // cleans up the terminal buffer
    }
    sti();
    return i;
}

