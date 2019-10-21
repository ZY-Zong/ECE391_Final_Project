/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* Initialize the 8259 PIC */
/*
 * i8259_init
 * This function initializes both slave and master PIC.
 * Input: None.
 * Output: None.
 * Side effect: Initialize the both PICs.
 */
void i8259_init(void) {

    // Disable all interrupts
    master_mask = 0xFF;
    slave_mask = 0xFF;

    // Mask all interrupts on both PIC
    outb(MASK_ALL_INTERRUPTS, MASTER_DATA);
    outb(MASK_ALL_INTERRUPTS, SLAVE_DATA);

    // Start initialize sequence
    outb(ICW1, MASTER_COMMAND);
    outb(ICW1, SLAVE_COMMAND);
    outb(ICW2_MASTER, MASTER_DATA);
    outb(ICW2_SLAVE, SLAVE_DATA);
    outb(ICW3_MASTER, MASTER_DATA);
    outb(ICW3_SLAVE, SLAVE_DATA);
    outb(ICW4, MASTER_DATA);
    outb(ICW4,SLAVE_DATA);

    // Restore the original mask
    outb(master_mask, MASTER_DATA);
    outb(slave_mask, SLAVE_DATA);

}

/* Enable (unmask) the specified IRQ */
/*
 * enable_irq
 * This function enables (unmasks) the specified IRQ.
 * Input: irq_num (Assume in range [0, 15]).
 * Output: None.
 * Side effect: Enable specified IRQ.
 */
void enable_irq(uint32_t irq_num) {
    uint8_t cur_mask;
    if (irq_num >= SLAVE_OFFSET) {
        // Unmask slave
        irq_num -= SLAVE_OFFSET;
        cur_mask = (inb(SLAVE_DATA)) & (~(MASK_ONE_BIT << irq_num));
        outb(cur_mask, SLAVE_DATA);
    } else {
        // Unmask master
        cur_mask = (inb(MASTER_DATA)) & (~(MASK_ONE_BIT << irq_num));
        outb(cur_mask, MASTER_DATA);
    }
}

/* Disable (mask) the specified IRQ */
/*
 * disable_irq
 * This function disables (masks) the specified IRQ.
 * Input: irq_num (Assume in range [0, 15]).
 * Output: None.
 * Side effect: Disable specified IRQ.
 */
void disable_irq(uint32_t irq_num) {
    uint8_t cur_mask;
    if (irq_num >= SLAVE_OFFSET) {
        // Mask slave
        irq_num -= SLAVE_OFFSET;
        cur_mask = inb(SLAVE_DATA) | (MASK_ONE_BIT << irq_num);
        outb(cur_mask, SLAVE_DATA);
    } else {
        // Mask master
        cur_mask = inb(MASTER_DATA) | (MASK_ONE_BIT << irq_num);
        outb(cur_mask, MASTER_DATA);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
/*
 * send_eoi
 * This function send end-of-interrupt signal for the specified IRQ.
 * Input: irq_num: *****Assumption****** Assume the irq_num is in range [0,15]
 * Output: None.
 * Side effect: Send an EOI to master and/or slave.
 */
void send_eoi(uint32_t irq_num) {
    if (irq_num >= SLAVE_OFFSET) {
        // Send EOI to slave
        irq_num -= SLAVE_OFFSET;
        outb((uint8_t)(EOI | irq_num), SLAVE_COMMAND);
        // Send EOI to master
        outb((uint8_t)(EOI | ICW3_SLAVE), MASTER_COMMAND);
    } else {
        // Only send EOI to master
        outb((uint8_t)(EOI | irq_num), MASTER_COMMAND);
    }
}

