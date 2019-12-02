/* idt_asm.S - Low-level handlers for exceptions, interrupts and system calls
*/

#define ASM     1
#include "idt.h"

.extern     system_sigreturn

/* Common macros */

.macro SETUP_HW_CONTEXT  # set up hardware context
	/* Not including Error Code / Dummy */
	/* Not including IRQ / Error # */
	push %fs
	push %es
	push %ds
	pushl %eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	pushl %ecx
	pushl %ebx
.endm

.macro RESTORE_HW_CONTEXT  /*  restore hardware context */
	popl %ebx
	popl %ecx
	popl %edx
	popl %esi
	popl %edi
	popl %ebp
	popl %eax
	pop %ds
	pop %es
	pop %fs
	addl $8, %esp  /* pop IRQ/Error# & ErrorCode/Dummy */
.endm


exception_unified_ll_handler:
    /* Error Code / Dummy has been pushed */
    /* Error # has been pushed */
    SETUP_HW_CONTEXT
    call unified_exception_handler
    call signal_check
    RESTORE_HW_CONTEXT
    iret

/* Low-level handlers (entry points) for exceptions */

/* Macro for generating code of exception entries */
.macro EXCEPTION_ENTRY n
.globl exception_entry_\n
exception_entry_\n:
    pushl $0   /* push dummy */
    pushl $\n  /* push exception number */
    jmp exception_unified_ll_handler
.endm

/* Macro for generating code of exception entries, including popping error code */
/* Reference: [Exceptions - OSDev Wiki](https://wiki.osdev.org/Exceptions#Alignment_Check) */
.macro EXCEPTION_ENTRY_WITH_ERRCODE n
.globl exception_entry_\n
exception_entry_\n:
    /* push Error Code is already on the stack */
    pushl $\n  /* push exception number */
    jmp exception_unified_ll_handler
.endm

EXCEPTION_ENTRY 0
EXCEPTION_ENTRY 1
EXCEPTION_ENTRY 2  /* non-maskable Interrupt */
EXCEPTION_ENTRY 3
EXCEPTION_ENTRY 4
EXCEPTION_ENTRY 5
EXCEPTION_ENTRY 6
EXCEPTION_ENTRY 7
EXCEPTION_ENTRY_WITH_ERRCODE 8
EXCEPTION_ENTRY 9
EXCEPTION_ENTRY_WITH_ERRCODE 10
EXCEPTION_ENTRY_WITH_ERRCODE 11
EXCEPTION_ENTRY_WITH_ERRCODE 12
EXCEPTION_ENTRY_WITH_ERRCODE 13
EXCEPTION_ENTRY_WITH_ERRCODE 14
/* 15 is reserved by Intel */
EXCEPTION_ENTRY 16
EXCEPTION_ENTRY_WITH_ERRCODE 17
EXCEPTION_ENTRY 18
EXCEPTION_ENTRY 19
EXCEPTION_ENTRY 20
/* 21-29 are for interrupts */
EXCEPTION_ENTRY_WITH_ERRCODE 30


/* Low-level handlers (entry points) for interrupts */

/* Macro for generating code of interrupt entries */
/* EOI must be handle manually in interrupt handler */
.macro INTERRUPT_ENTRY irq, func
.globl interrupt_entry_\irq
interrupt_entry_\irq:
    pushl $0       /* push Dummy */
    pushl $\irq    /* push irq number */
    SETUP_HW_CONTEXT
    call \func
    call signal_check
    RESTORE_HW_CONTEXT
    iret  /* return from interrupt context */
.endm

INTERRUPT_ENTRY 0, sched_pit_interrupt_handler
INTERRUPT_ENTRY 1, keyboard_interrupt_handler
INTERRUPT_ENTRY 8, rtc_interrupt_handler


/* Low-level handlers (entry points) for system calls */

.globl system_call_entry
system_call_entry:
    pushl $0       /* push Dummy */
    pushl $0x80    /* push vec number */
    SETUP_HW_CONTEXT
    /* If SYSTEM_CALL_TABLE_SIZE <= EAX, call sys_not_implemented */
    cmpl $SYSTEM_CALL_TABLE_SIZE, %eax
    jae system_call_entry_invalid
    /* If EAX valid, call corresponding system call */
    /* HW context on the stack matches arguments */
    call *system_call_table(,%eax,4)
    jmp system_call_entry_done
system_call_entry_invalid:
    call sys_not_implemented
system_call_entry_done:
    movl %eax, 24(%esp)  /* store EAX to HW context, allow it to immigrate with signal functions */
    call signal_check
    RESTORE_HW_CONTEXT  /* new EAX has been written into it */
    iret

system_call_table:
    .long sys_not_implemented  /* 0 */
    .long lowlevel_sys_halt
    .long lowlevel_sys_execute
    .long lowlevel_sys_read
    .long lowlevel_sys_write
    .long lowlevel_sys_open    /* 5 */
    .long lowlevel_sys_close
    .long lowlevel_sys_getargs
    .long lowlevel_sys_vidmap
    .long lowlevel_sys_set_handler
    .long system_sigreturn  /* 10 */
