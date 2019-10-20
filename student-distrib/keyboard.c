/*									tab:8
 *
 * keyboard.c - source file for keyboard device
 * Author:	    Qi Gao
 *              Tingkai Liu
 *              Zikai Liu
 *              Zhenyu Zong
 * Creation Date:   Sat Oct 19 22:34 2019
 * Filename:        keyboard.c
 */

#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

/* Keys that correspond to scan codes, using scan code set 1 for "US QWERTY" keyboard
 * REFERENCE: https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Sets.2C_Scan_Codes_and_Key_Codes
 */
static char scan_code_table[128] {
        0,  0 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  0 ,      /* 0x00 - 0x0E */
        0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'o', 'p', '[', ']', '\n',               /* 0x0F - 0x1C */
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',                /* 0x1D - 0x29 */
        0, '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',  0 ,  0 ,           /* 0x2A - 0x37 */
        0, ' ',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,      /* 0x38 - 0x46 */
        0,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,                /* 0x47 - 0x53 */
        0,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
        0,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,
        0,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0                  /* 0x54 - 0x80 */
};

/*
 * keyboard_init
 *   DESCRIPTION: Initialize the keyboard device
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void keyboard_init() {
    /* Enable the keyboard IRQ */
    enable_irq(KEYBOARD_IRQ);
}


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

    /* Get scan code from port 0x60 */
    uint8_t scancode = inb(KEYBOARD_PORT);

    /* Output the char to the console */
    putc(scan_code_table[scancode]);

    sti();
}

