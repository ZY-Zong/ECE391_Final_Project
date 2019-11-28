//
// Created by qig2 on 10/26/2019.
//
#include "terminal.h"

#include "task.h"
#include "task_sched.h"
#include "idt.h"

#define KEYBOARD_PORT   0x60    /* keyboard scancode port */
#define KEYBOARD_FLAG_SIZE 128

#define CTRL_PRESS 0x1D
#define ALT_PRESS 0x38
#define LEFT_SHIFT_PRESS 0x2A
#define RIGHT_SHIFT_PRESS 0x36
#define BACKSPACE_PRESS 0x0E
#define CAPSLOCK_PRESS 0x3A
#define ENTER_PRESS 0x1C
#define L_PRESSED 0x26
#define C_PRESSED 0x2E
#define F1_PRESS 0x3B

// Temporary height and width for text mode
#define TEXT_MODE_WIDTH 80
#define TEXT_MODE_HEIGHT 25

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

// Local wait list for the terminals
task_list_node_t terminal_wait_list = TASK_LIST_SENTINEL(terminal_wait_list);
terminal_t terminal_slot[TERMINAL_MAX_COUNT];

void handle_scan_code(uint8_t scan_code);

#define KEYBOARD_F1_SCANCODE 0x3B
#define KEYBOARD_F2_SCANCODE 0x3C
#define KEYBOARD_F3_SCANCODE 0x3D
#define KEYBOARD_SCANCODE_PRESSED 0x80


/**
 * Keyboard interrupt handler
 * @param irq_num    Keyboard interrupt irq number, used for sending EOI
 * @usage idt_asm.S
 * @note Reference: https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Sets.2C_Scan_Codes_and_Key_Codes
 */
asmlinkage void keyboard_interrupt_handler(uint32_t irq_num) {

    uint32_t flags = 0;

    cli_and_save(flags);
    {
        // Get scan code from port 0x60
        uint8_t scancode = inb(KEYBOARD_PORT);

        // After read from keyboard, send EOI
        idt_send_eoi(irq_num);

        if (scancode == KEYBOARD_F1_SCANCODE) {  // F1
            clear();
        } else {
            handle_scan_code(scancode);  // output the char to the console
        }
    }
    restore_flags(flags);
}

/**
 * This function handles scancode from the keyboard and stores the current character into the keyboard buffer
 * @param scan_code    Scan code to be handled
 * @note If the keyboard buffer is full, just discard any keystroke from the keyboard
 * @note Priority of the keys: Enter > CapsLock > Ctrl+L > Ctrl+Fn > Backspace
 */
void handle_scan_code(uint8_t scan_code) {

    if (scan_code >= KEYBOARD_SCANCODE_PRESSED) {
        // If the scan_code is a release code, just reset the flags
        key_flags[scan_code - KEYBOARD_SCANCODE_PRESSED] = 0;
        
    } else {
        
        // If the scan_code is a press code, set the flags
        key_flags[scan_code] = 1;

        // If Enter and someone is reading from the keyboard
        terminal_t *focus_term = focus_task()->terminal;

        if ((0 != focus_term->user_ask_len) && (ENTER_PRESS == scan_code)) {

            focus_term->key_buf[focus_term->key_buf_cnt] = '\n';
            putc('\n');
            focus_term->key_buf_cnt++;

            // Wake up the sleep task
            // No lock is needed, since this function is already placed in a lock
            focus_task()->flags &= ~TASK_WAITING_TERMINAL;
            sched_refill_time(focus_task());
            sched_insert_to_head(focus_task());
            sched_launch_to_current_head();
            // Return after this task is active again...
            return;
        }

        // If Enter and no one is reading from the keyboard, clear the keyboard buffer
        if ((0 == focus_term->user_ask_len) && (ENTER_PRESS == scan_code)) {
            focus_term->key_buf_cnt = 0;
            putc('\n');
            return;
        }

        // If CapsLock
        if (CAPSLOCK_PRESS == scan_code) {
            capslock_status ^= 0x1; //  Flip the status bit for CapsLock
            return;
        }

        // If Ctrl+C
        if (1 == key_flags[CTRL_PRESS] && 1 == key_flags[C_PRESSED]){
            // TODO: Wait for Tingkai's function
            return;
        }

        // If Ctrl+L
        if (1 == key_flags[CTRL_PRESS] && 1 == key_flags[L_PRESSED]) {
            clear(); // clear the screen
            reset_cursor(); // reset the cursor to up-left corner

            // Keep the last typed line
            printf("391OS>");
            int i;
            for (i = 0; i < focus_term->key_buf_cnt; i++) {
                putc(focus_term->key_buf[i]);
            }
            return;
        }

        // If Alt+F(1-3) to change terminals
        // Here only support 3 terminals, and terminal index starts from 0
        if (1 == key_flags[ALT_PRESS]) {
            int i;  // Loop counter for the F1-F3
            for (i = 0; i < 3; i++) {
                if (1 == key_flags[F1_PRESS + i]) {
                    task_change_focus(i);
                    break;
                }
            }
        }

        // If backspace
        // Just putc then delete the char in the key_buf
        if (1 == key_flags[BACKSPACE_PRESS]) {
            if (0 < focus_term->key_buf_cnt) {
                putc('\b');
                focus_term->key_buf_cnt--;
            }
        } else {

            char character;

            if (!capslock_status && (key_flags[LEFT_SHIFT_PRESS] || key_flags[RIGHT_SHIFT_PRESS])) {
                // If not capslock but shift is pressed
                character = shift_scan_code_table[scan_code];
            } else if (!capslock_status && (!key_flags[LEFT_SHIFT_PRESS] && !key_flags[RIGHT_SHIFT_PRESS])){
                // If neither capslock nor shift is pressed
                character = scan_code_table[scan_code];
            } else if (capslock_status && (key_flags[LEFT_SHIFT_PRESS] || key_flags[RIGHT_SHIFT_PRESS])) {
                // If both capslock and shift is pressed
                character = caps_shift_scan_code_table[scan_code];
            } else {
                // If capslock but shift is not pressed
                character = caps_scan_code_table[scan_code];
            }

            if (0 != character) {
                if (focus_term->key_buf_cnt < KEYBOARD_BUF_SIZE - 1) {
                    focus_term->key_buf[focus_term->key_buf_cnt] = character;
                    putc(character);
                    focus_term->key_buf_cnt++;
                }
            }

        }
        // If we reached the length user wants, return
        if (focus_term->user_ask_len > 0 && focus_term->key_buf_cnt >= focus_term->user_ask_len) {
            // Wake up the sleep task
            // No lock is needed, since this function is already placed in a lock
            focus_task()->flags &= ~TASK_WAITING_TERMINAL;
            sched_refill_time(focus_task());
            sched_insert_to_head(focus_task());
            sched_launch_to_current_head();
            // Return after this task is active again...
        }
    }
}

/**
 * System call implementation for terminal open
 * @param filename    No use
 * @return 0
 */
int32_t system_terminal_open(const uint8_t *filename) {
    (void) filename;
    return 0;
}

/**
 * System call implementation for terminal close
 * @param fd    No use
 * @return 0
 */
int32_t system_terminal_close(int32_t fd) {
    (void) fd;
    return 0;
}

/**
 * System call implementation for terminal read
 * @param fd        File descriptor, must be 0
 * @param buf       Buffer to store output
 * @param nbytes    Maximal number of bytes to write
 * @return When enter is pressed or nbytes (< 128) is reached
 */
int32_t system_terminal_read(int32_t fd, void *buf, int32_t nbytes) {
    int32_t i = 0;  // record how many characters are read from the buffer before we reach nbytes, key_buf_size or '\n'
    int32_t j;  // counter
    int32_t to_delete = 0;
    int32_t to_continue = 1;
    uint32_t flags;

    if (fd != 0) {
        DEBUG_ERR("system_terminal_read(): invalid fd %d for terminal read", fd);
        return -1;
    }

    // If user asks more than 128 bytes of data, then just return 128 bytes.
    if (nbytes > KEYBOARD_BUF_SIZE) {
        nbytes = KEYBOARD_BUF_SIZE;
    }

    running_task()->terminal->user_ask_len = nbytes;

    cli_and_save(flags);
    {
        // Perform full scan, in case key buffer changes in an unexpected way.
        // For example, an backspace and an enter are pressed during two loops, we need to identify that enter.
        for (i = 0; (i < running_task()->terminal->key_buf_cnt) && (i < nbytes); i++) {
            if (running_task()->terminal->key_buf[i] == '\n') {
                to_continue = 0;
                break;  // exit for
            }
        }
        if (i == nbytes) {  // already fulfill buffer given by user, return immediately
            to_continue = 0;
        }

    }
    restore_flags(flags);

    if (1 == to_continue) {
        // Set the task to sleep
        cli_and_save(flags);
        {
            running_task()->flags |= TASK_WAITING_TERMINAL;
            sched_move_running_after_node(&terminal_wait_list);
            sched_launch_to_current_head();
            // Return after this task is active again...
        }
        restore_flags(flags);
    }

    running_task()->terminal->user_ask_len = 0;  // serves the same function as whether_read

    cli_and_save(flags);
    {
        memcpy(buf, running_task()->terminal->key_buf, i);
        to_delete = i + (running_task()->terminal->key_buf[i] == '\n');
        for (j = to_delete; j < running_task()->terminal->key_buf_cnt; j++) {
            running_task()->terminal->key_buf[j - to_delete] = running_task()->terminal->key_buf[j];
        }
        running_task()->terminal->key_buf_cnt -= to_delete;
    }
    restore_flags(flags);

    return i;
}

/**
 * System call implementation for terminal write
 * @param fd        File descriptor, must be 1
 * @param buf       Buffer of content to write
 * @param nbytes    Number of bytes to write
 * @return 0 on success, -1 on failure
 */
int32_t system_terminal_write(int32_t fd, const void *buf, int32_t nbytes) {
    int i;
    uint32_t write_flags = 0;

    if (fd != 1) {
        DEBUG_ERR("system_terminal_write(): invalid fd %d for terminal write", fd);
        return -1;
    }

    // Critical section to prevent keyboard buffer changes during the copy operation.
    cli_and_save(write_flags);
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
    restore_flags(write_flags);
    return i;
}

/**
 * Initialize terminal control
 */
void terminal_init() {
    int i;
    for (i = 0; i < TERMINAL_MAX_COUNT; i++) {
        terminal_slot[i].valid = 0;
    }
}

/**
 * Allocate a new terminal control block. No video memory included
 * @return Pointer to a new terminal control block on success, NULL on failure
 */
terminal_t *terminal_allocate() {
    int i;

    // Find available slot
    for (i = 0; i < TERMINAL_MAX_COUNT; i++) {
        if (terminal_slot[i].valid == 0) break;
    }

    if (i >= TERMINAL_MAX_COUNT) {
        DEBUG_ERR("terminal_allocate(): no available slot for new terminal");
        return NULL;
    }

    terminal_t *terminal = &terminal_slot[i];
    terminal->valid = 1;
    terminal->terminal_id = i;

    memset(terminal->key_buf, 0, sizeof(terminal->key_buf));
    terminal->key_buf_cnt = 0;

    terminal->screen_width = TEXT_MODE_WIDTH;
    terminal->screen_height = TEXT_MODE_HEIGHT;
    terminal->screen_x = 0;
    terminal->screen_y = 0;

    return terminal;
}

/**
 * Deallocate a terminal control block. No action on video memory is included.
 * @param terminal    Pointer to the terminal control
 */
void terminal_deallocate(terminal_t* terminal) {
    terminal->valid = 0;
}

