/* idt_asm.S - Low-level handlers for exceptions, interrupts and system calls
*/

#define ASM     1
#include "idt.h"

/* Common macros */

.macro SAVE_ALL_REGS  # save all register onto the stack
	pushf
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
.endm

.macro RESTORE_ALL_REGS  # restore all register from the stack
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popf
.endm


/* Low-level handlers (entry points) for exceptions */

# Macro for generating code of exception entries
.macro EXCEPTION_ENTRY n, func
.globl exception_entry_\n
exception_entry_\n:
    SAVE_ALL_REGS
    pushl $\n  # push exception number
    call \func
    addl $4, %esp  # pop arg
    RESTORE_ALL_REGS
    iret  # return from interrupt context
.endm

# Macro for generating code of exception entries, including popping error code
# Reference: [Exceptions - OSDev Wiki](https://wiki.osdev.org/Exceptions#Alignment_Check)
.macro EXCEPTION_ENTRY_WITH_ERRCODE n, func
.globl exception_entry_\n
exception_entry_\n:
    SAVE_ALL_REGS
    pushl $\n  # push exception number
    call \func
    addl $4, %esp  # pop arg
    RESTORE_ALL_REGS
    addl $4, %esp  # pop error code pushed by processor
    iret  # return from interrupt context
.endm

EXCEPTION_ENTRY 0, print_exception
EXCEPTION_ENTRY 1, print_exception
EXCEPTION_ENTRY 2, print_exception  # non-maskable Interrupt
EXCEPTION_ENTRY 3, print_exception
EXCEPTION_ENTRY 4, print_exception
EXCEPTION_ENTRY 5, print_exception
EXCEPTION_ENTRY 6, print_exception
EXCEPTION_ENTRY 7, print_exception
EXCEPTION_ENTRY_WITH_ERRCODE 8, print_exception
EXCEPTION_ENTRY 9, print_exception
EXCEPTION_ENTRY_WITH_ERRCODE 10, print_exception
EXCEPTION_ENTRY_WITH_ERRCODE 11, print_exception
EXCEPTION_ENTRY_WITH_ERRCODE 12, print_exception
EXCEPTION_ENTRY_WITH_ERRCODE 13, print_exception
EXCEPTION_ENTRY_WITH_ERRCODE 14, print_exception
/* 15 is reserved by Intel */
EXCEPTION_ENTRY 16, print_exception
EXCEPTION_ENTRY_WITH_ERRCODE 17, print_exception
EXCEPTION_ENTRY 18, print_exception
EXCEPTION_ENTRY 19, print_exception
EXCEPTION_ENTRY 20, print_exception
/* 21-29 are for interrupts */
EXCEPTION_ENTRY_WITH_ERRCODE 30, print_exception


/* Low-level handlers (entry points) for interrupts */

/* Macro for generating code of interrupt entries */
/* EOI will be sent after return of interrupt function */
.macro INTERRUPT_ENTRY irq, func
.globl interrupt_entry_\irq
interrupt_entry_\irq:
    SAVE_ALL_REGS
    call \func
    pushl $\irq    /* push irq number */
    call send_eoi  /* defined in i8259.c/h */
    addl $4, %esp  /* pop arg */
    RESTORE_ALL_REGS
    iret  /* return from interrupt context */
.endm

INTERRUPT_ENTRY 1, keyboard_interrupt_handler
INTERRUPT_ENTRY 8, rtc_interrupt_handler


/* Low-level handlers (entry points) for system calls */

.globl system_call_entry
system_call_entry:
    pushf
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
    /* if SYSTEM_CALL_TABLE_SIZE <= EAX, call sys_not_implemented */
    cmpl $SYSTEM_CALL_TABLE_SIZE, %eax
    jae system_call_entry_invalid
    /* if EAX valid, call corresponding system call */
    call *system_call_table(,%eax,4)
    jmp system_call_entry_done
system_call_entry_invalid:
    call sys_not_implemented
system_call_entry_done:
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    popf
    iret  /* return from interrupt context */

system_call_table:
    .long sys_not_implemented  /* 0 */
    .long lowlevel_sys_halt
    .long lowlevel_sys_execute
    .long lowlevel_sys_read
    .long lowlevel_sys_write
    .long lowlevel_sys_open             /* 5 */
    .long lowlevel_sys_close
    .long lowlevel_sys_getargs
    .long sys_not_implemented
    .long sys_not_implemented
    .long sys_not_implemented  /* 10 */
