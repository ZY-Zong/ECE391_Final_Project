/* idt_handler.S - Kernel functions for exceptions, interrupts and system calls
*/

#include "idt_handler.h"
#include "lib.h"
#include "linkage.h"
#include "errno.h"

void null_interrupt_handler(uint32_t irq) {

    cli();
    {
        printf(PRINT_ERR "Interrupt handler for IRQ %u is not implemented", irq);
    }
    sti();
}


/** System Call Handlers */

/**
 * For not implemented system call
 * @return -ENOSYS
 */
long sys_not_implemented() {
    return -ENOSYS;
}

asmlinkage int32_t sys_read(int32_t fd, void* buf, int32_t nbytes) {

}

asmlinkage int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes) {

}

asmlinkage int32_t sys_open(const uint8_t* filename) {

}

asmlinkage int32_t sys_close(int32_t fd) {

}