# boot.S - start point for the kernel after GRUB gives us control
# vim:ts=4 noexpandtab

#define ASM     1
#define ENABLE_PAGING   1

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

// TODO: add function document
.globl enable_paging
enable_paging:

    enter $0, $0

    # EAX and ECX are caller saved

    # mp3.1 paging: the space for PD and PT
    # Get the address of the table
    movl    $kernel_page_table_0, %eax  # get the address of label 0
    andl    $0xFFFFF000, %eax   # get high 20 bits, not needed since it is 4kB align. do anyway
    movl    kernel_page_directory, %ecx     # index 0 of PD
    orl     %eax, %ecx
    movl    %ecx, kernel_page_directory

    # Enable paging

    # set cr3 (PDBR)
    /*
     * the high 20 bits of cr3 is the base address
     */
    movl    $kernel_page_directory, %eax    # get the address of the PD

    # get high 20 bits
    andl    $0xFFFFF000, %eax   # not necessary since it is 4kB align, but do anyway
    # store it in cr3
    movl    %cr3, %ecx
    orl     %eax, %ecx
    movl    %ecx, %cr3


    # set cr4 (bit4: PSE)
    movl    %cr4, %eax
    orl     $0x10, %eax
    movl    %eax, %cr4

    /*
    * paging flag: bit 31 of CR0
    * protection enable: bit 0 of CR0
    */
    movl    %cr0, %eax
    orl     $0x80000000, %eax
    movl    %eax, %cr0

    # fill the PD and PT
    # call    init_paging     already done in version 1.1

    leave
    ret


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
    pushl $\n  # push exception number
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


