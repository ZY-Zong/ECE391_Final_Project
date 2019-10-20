/*									tab:8
 *
 * keyboard.h - header file for keyboard device
 * Author:	    Qi Gao
 *              Tingkai Liu
 *              Zikai Liu
 *              Zhenyu Zong
 * Creation Date:   Sat Oct 19 22:33 2019
 * Filename:        keyboard.h
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEYBOARD_IRQ    1   /* keyboard IRQ number */
#define KEYBOARD_PORT   0x60    /* keyboard scancode port */

/* Initialize keyboard - enable its IRQ */
void keyboard_init();

/* Handle keyboard interrupt */
void keyboard_interrupt_handler();

#endif /* KEYBOARD_H */
