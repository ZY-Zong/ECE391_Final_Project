/* idt_handler.S - Kernel functions for exceptions, interrupts and system calls
*/

#include "lib.h"

void null_interrupt_handler(uint32_t irq) {

    cli();
    {
        printf(PRINT_ERR "Interrupt handler for IRQ %u is not implemented", irq);
    }
    sti();
}