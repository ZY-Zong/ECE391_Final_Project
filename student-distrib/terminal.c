//
// Created by qig2 on 10/26/2019.
//
#include "terminal.h"

#include "task.h"

#define KEYBOARD_PORT   0x60    /* keyboard scancode port */
#define KEYBOARD_FLAG_SIZE 128
#define CTRL_PRESS 0x1D
#define CTRL_RELEASE 0x9D
#define LEFT_SHIFT_PRESS 0x2A
#define LEFT_SHIFT_RELEASE 0xAA
#define RIGHT_SHIFT_PRESS 0x36
#define RIGHT_SHIFT_RELEASE 0xB6
#define BACKSPACE_SCAN_CODE 0x0E
#define CAPSLOCK_PRESS 0x3A
#define ENTER_PRESS 0x1C
#define L_SCANCODE_PRESSED 0x26

/* Keys that correspond to scan codes, using scan code set 1 for "US QWERTY" keyboard
 * REFERENCE: https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Sets.2C_Scan_Codes_and_Key_Codes
 * TODO: Not handled keys: Esc, Tab, right ctrl.
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
static const char caps_shift_scan_code_table[128] = {
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',   /* 0x00 - 0x0E */
        0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n',      /* 0x0F - 0x1C */
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '\"', '~',           /* 0x1D - 0x29 */
        0, '|', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0, 0,          /* 0x2A - 0x37 */
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            /* 0x38 - 0x46 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    /* 0x47 - 0x53 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        /* 0x54 - 0x80 */
};
static const char caps_scan_code_table[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',      /* 0x00 - 0x0E */
        0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',      /* 0x0F - 0x1C */
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',           /* 0x1D - 0x29 */
        0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, 0,           /* 0x2A - 0x37 */
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            /* 0x38 - 0x46 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    /* 0x47 - 0x53 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                        /* 0x54 - 0x80 */
};
// Array to record what keys has been pressed currently
static uint8_t key_flags[KEYBOARD_FLAG_SIZE];
// Bit vector used to check whether the CapsLock is on
static uint8_t capslock_status = 0;
// Bit vector used to check whether someone is reading from the keyboard
static uint8_t whether_read = 0;
// Keyboard buffer of size 128 and a counter to store the current position in the buffer
static char keyboard_buf[KEYBOARD_BUF_SIZE];
static uint8_t keyboard_buf_counter = 0;
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
        } else {
            handle_scan_code(scancode);  // output the char to the console
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
//        printf("[%x]", scan_code - KEYBOARD_SCANCODE_PRESSED);
    } else {
        // If the scan_code is a press code, set the flags
        key_flags[scan_code] = 1;
//        printf("(%x)", scan_code);
        // If Enter and someone is reading from the keyboard, just let the read function handle \n
        if ((1 == whether_read) && (ENTER_PRESS == scan_code)) {
            keyboard_buf[keyboard_buf_counter] = '\n';
            putc(keyboard_buf[keyboard_buf_counter]);
            keyboard_buf_counter++;
            return;
        }
        // If Enter and no one is reading from the keyboard, clear the keyboard buffer
        if ((0 == whether_read) && (ENTER_PRESS == scan_code)) {
            keyboard_buf_counter = 0;
            putc('\n');
            return;
        }
        // If CapsLock
        if (CAPSLOCK_PRESS == scan_code) {
            capslock_status ^= 0x1; //  Flip the status bit for CapsLock
            return;
        }
        // If Ctrl+L
        if (1 == key_flags[CTRL_PRESS] && 1 == key_flags[L_SCANCODE_PRESSED]) {
            clear(); // clear the screen
            reset_cursor(); // reset the cursor to up-left corner
            return;
        }
        // If backspace
        // Just putc then delete the char in the keyboard_buf
        if (1 == key_flags[BACKSPACE_SCAN_CODE]) {
            if (0 < keyboard_buf_counter) {
                putc('\b');
                keyboard_buf_counter--;
            }
        } else {
            // If shift is pressed
            if ((0 == capslock_status) && ((1 == key_flags[LEFT_SHIFT_PRESS]) || (1 == key_flags[RIGHT_SHIFT_PRESS]))) {
                // If CapsLock but shift is pressed
                if (0 != shift_scan_code_table[scan_code]) {
                    if (keyboard_buf_counter < KEYBOARD_BUF_SIZE - 1) {
                        keyboard_buf[keyboard_buf_counter] = shift_scan_code_table[scan_code];
                        putc(keyboard_buf[keyboard_buf_counter]);
                        keyboard_buf_counter++;
                    }
                }
            } else if ((0 == capslock_status) && ((0 == key_flags[LEFT_SHIFT_PRESS]) && (0 == key_flags[RIGHT_SHIFT_PRESS]))){
                // If not CapsLock and shift is not pressed
                if (0 != scan_code_table[scan_code]) {
                    if (keyboard_buf_counter < KEYBOARD_BUF_SIZE - 1) {
                        keyboard_buf[keyboard_buf_counter] = scan_code_table[scan_code];
                        putc(keyboard_buf[keyboard_buf_counter]);
                        keyboard_buf_counter++;
                    }
                }
            } else if ((1 == capslock_status) && ((1 == key_flags[LEFT_SHIFT_PRESS]) || (1 == key_flags[RIGHT_SHIFT_PRESS]))) {
                // If CapsLock and shift is pressed
                if (0 != scan_code_table[scan_code]) {
                    if (keyboard_buf_counter < KEYBOARD_BUF_SIZE - 1) {
                        keyboard_buf[keyboard_buf_counter] = caps_shift_scan_code_table[scan_code];
                        putc(keyboard_buf[keyboard_buf_counter]);
                        keyboard_buf_counter++;
                    }
                }
            } else {
                // If CapsLock but shift is not pressed
                if (0 != scan_code_table[scan_code]) {
                    if (keyboard_buf_counter < KEYBOARD_BUF_SIZE - 1) {
                        keyboard_buf[keyboard_buf_counter] = caps_scan_code_table[scan_code];
                        putc(keyboard_buf[keyboard_buf_counter]);
                        keyboard_buf_counter++;
                    }
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
    int32_t to_continue = 1;

    if (fd != 0) {
        DEBUG_ERR("terminal_read(): invalid fd %d for terminal read\n", fd);
        return -1;
    }

    // If user asks more than 128 bytes of data, then just return 128 bytes.
    if (nbytes > KEYBOARD_BUF_SIZE) {
        nbytes = KEYBOARD_BUF_SIZE;
    }
    whether_read = 1;
    while (to_continue) {
        cli();
        {
            // Perform full scan, in case key buffer changes in an unexpected way.
            // For example, an backspace and an enter are pressed during two loops, we need to identify that enter.
            for (i = 0; (i < keyboard_buf_counter) && (i < nbytes); i++) {
                if (keyboard_buf[i] == '\n') {
                    to_continue = 0;
                    break;  // exit for
                }
            }
            if (i == nbytes) {  // already fulfill buffer given by user, return immediately
                to_continue = 0;
            }
        }
        sti();
    }
    whether_read = 0;
    cli();
    {
        memcpy(buf, keyboard_buf, i);
        to_delete = i + (keyboard_buf[i] == '\n');
        for (j = to_delete; j < keyboard_buf_counter; j++) {
            keyboard_buf[j - to_delete] = keyboard_buf[j];
        }
        keyboard_buf_counter -= to_delete;
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

    if (fd != 1) {
        DEBUG_ERR("terminal_write(): invalid fd %d for terminal write\n", fd);
        return -1;
    }

    // Critical section to prevent keyboard buffer changes during the copy operation.
    cli();
    {
        for (i = 0; i < nbytes; i++) {
            // TODO: decide whether to terminate write when seeing a NUL
//            if (0 == ((uint8_t *) buf)[i]) {
//                // If current character is '\0', stop
//                break;
//            }
            putc(((uint8_t *) buf)[i]);
        }
    }
    sti();
    return i;
}

