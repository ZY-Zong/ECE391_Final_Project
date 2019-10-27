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

asmlinkage long sys_read(unsigned int fd, char __user *buf, size_t count) {

}

asmlinkage long sys_write(unsigned int fd, const char __user *buf, size_t count) {

}

asmlinkage long sys_open(const char __user *filename, int flags, int mode) {

}

asmlinkage long sys_close(unsigned int fd) {

}