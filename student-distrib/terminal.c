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
                putc(scan_code_table[scancode]);  // output the char to the console
            }
#ifdef RUN_TESTS
            test1_handle_typing(scan_code_table[scancode]);
#endif
        }
    }
    sti();
}

