/*									tab:8
 *
 * RTC.c - source file for Real Time Clock device
 * Author:	    Qi Gao
 *              Tingkai Liu
 *              Zikai Liu
 *              Zhenyu Zong
 * Creation Date:   Sat Oct 19 23:53 2019
 * Filename:        RTC.c
 */

#include "RTC.h"
#include "lib.h"
#include "i8259.h"

/*
 * rtc_init
 *   REFERENCE: https://wiki.osdev.org/RTC
 *   DESCRIPTION: Initialize the real time clock
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Frequency is set to be default 1024 Hz
 */
void rtc_init() {
    /* Turn on IRQ 8, default frequency is 1024 Hz*/
    cli();  // disable interrupts
    outb(RTC_STATUS_REGISTER_B, RTC_REGISTER_PORT); // select register B and disable NMI
    char prev = inb(RTC_RW_DATA_PORT);	// read the current value of register B
    outb(RTC_STATUS_REGISTER_B, RTC_REGISTER_PORT);	// set the index again (a read will reset the index to register D)
    outb (prev | 0x40, RTC_RW_DATA_PORT);	// write the previous value ORed with 0x40. This turns on bit 6 of register B
    sti();

    /* Enable the keyboard IRQ */
    enable_irq(RTC_IRQ);
}

/*
 * rtc_interrupt_handler
 *   REFERENCE: https://wiki.osdev.org/RTC
 *   DESCRIPTION: Handle the RTC interrupt. Note that Status Register C will contain a bitmask
 *                telling which interrupt happened. If register C is not read after an IRQ 8,
 *                then the interrupt will not happen again.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void rtc_interrupt_handler() {
    /* Get another interrupt */
    outb(RTC_STATUS_REGISTER_C, RTC_REGISTER_PORT);	// select register C
    inb(RTC_RW_DATA_PORT); // just throw away contents

    /* Send EOI */
    send_eoi(RTC_IRQ);
}