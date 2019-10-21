# boot.S - start point for the kernel after GRUB gives us control
# vim:ts=4 noexpandtab

#define ASM     1

#include "multiboot.h"
#include "x86_desc.h"

.text

    # Multiboot header (required for GRUB to boot us)
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS
    .long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

# Entrypoint to the kernel
.globl start, _start


.align 4
start:
_start:
    # Make sure interrupts are off
    cli
    jmp     continue

continue:
    # Load the GDT
    lgdt    gdt_desc_ptr
    # Load CS with the new descriptor value
    ljmp    $KERNEL_CS, $keep_going

keep_going:
    # Set up ESP so we can have an initial stack
    movl    $0x800000, %esp

    # Set up the rest of the segment selector registers
    movw    $KERNEL_DS, %cx
    movw    %cx, %ss
    movw    %cx, %ds
    movw    %cx, %es
    movw    %cx, %fs
    movw    %cx, %gs

    # Push the parameters that entry() expects (see kernel.c):
    # eax = multiboot magic
    # ebx = address of multiboot info struct
    pushl   %ebx
    pushl   %eax

    # Jump to the C entrypoint to the kernel.
    call    entry

    # We'll never get back here, but we put in a hlt anyway.
halt:
    hlt
    jmp     halt

#define SAVE_ALL_REGS \
	pushl %ebp; \
	pushl %edi; \
	pushl %esi; \
	pushl %edx; \
	pushl %ecx; \
	pushl %ebx; \
	pushl %eax;

#define RESTORE_ALL_REGS \
	popl %eax; \
	popl %ebx; \
	popl %ecx; \
	popl %edx; \
	popl %esi; \
	popl %edi; \
	popl %ebp;

.macro DEF_EXCEPTION_ENTRY n, func
.globl exception_entry_\n
exception_entry_\n:
    SAVE_ALL_REGS
    pushl $\n  # push irq number
    call \func
    addl $4, %esp  # pop arg
    RESTORE_ALL_REGS
    iret  # return from interrupt context
.endm

DEF_EXCEPTION_ENTRY 0, print_exception  # Divide Error Exception
DEF_EXCEPTION_ENTRY 1, print_exception
DEF_EXCEPTION_ENTRY 2, print_exception
DEF_EXCEPTION_ENTRY 3, print_exception
DEF_EXCEPTION_ENTRY 4, print_exception
DEF_EXCEPTION_ENTRY 5, print_exception
DEF_EXCEPTION_ENTRY 6, print_exception
DEF_EXCEPTION_ENTRY 7, print_exception
DEF_EXCEPTION_ENTRY 8, print_exception
DEF_EXCEPTION_ENTRY 9, print_exception
DEF_EXCEPTION_ENTRY 10, print_exception
DEF_EXCEPTION_ENTRY 11, print_exception
DEF_EXCEPTION_ENTRY 12, print_exception
DEF_EXCEPTION_ENTRY 13, print_exception
DEF_EXCEPTION_ENTRY 14, print_exception
# 15 is reserved by Intel
DEF_EXCEPTION_ENTRY 16, print_exception
DEF_EXCEPTION_ENTRY 17, print_exception
DEF_EXCEPTION_ENTRY 18, print_exception
DEF_EXCEPTION_ENTRY 19, print_exception

.macro DEF_INTERRUPT_ENTRY irq, func
.globl interrupt_entry_\irq
interrupt_entry_\irq:
    SAVE_ALL_REGS
    call \func
    pushl $\irq  # push irq number
    call send_eoi
    addl $4, %esp  # pop arg
    RESTORE_ALL_REGS
    iret  # return from interrupt context
.endm

DEF_INTERRUPT_ENTRY 1, keyboard_interrupt_handler
DEF_INTERRUPT_ENTRY 8, rtc_interrupt_handler


