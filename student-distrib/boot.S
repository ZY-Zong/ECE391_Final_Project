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

.macro IRQ_ENTRY n, func
.globl irq_entry_\n
irq_entry_\n:
    SAVE_ALL_REGS
    pushl $\n  # push irq number
    call \func
    call send_eoi
    addl $4, %esp  # pop arg
    RESTORE_ALL_REGS
    iret  # return from interrupt context
.endm

IRQ_ENTRY 0, print_exception  # Divide Error Exception
IRQ_ENTRY 1, print_exception
IRQ_ENTRY 2, print_exception
IRQ_ENTRY 3, print_exception
IRQ_ENTRY 4, print_exception
IRQ_ENTRY 5, print_exception
IRQ_ENTRY 6, print_exception
IRQ_ENTRY 7, print_exception
IRQ_ENTRY 8, print_exception
IRQ_ENTRY 9, print_exception
IRQ_ENTRY 10, print_exception

IRQ_ENTRY 21, keyboard_interrupt_handler
IRQ_ENTRY 28, rtc_interrupt_handler
IRQ_ENTRY 80, print_exception

