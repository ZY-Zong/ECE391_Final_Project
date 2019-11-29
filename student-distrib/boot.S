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
    # Set up ESP so we can have an initial stack, 8MB
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


/**
 * enable_paging
 * This function is used to setting up page directory and page table for kernel. Called in kernel.c.
 * Input: None.
 * Output: None.
 * Side effect: Enable paging.
 * Note: Use C calling convension
 */
.globl enable_paging
enable_paging:

    # EAX and ECX are caller saved
    # Other callee-saved registers are not used

    enter $0, $0

    # Write the table for first 4K into entry 0 of page directory
    movl    $kernel_page_table_0, %eax   # get the address of label 0
    # andl    $0xFFFFF000, %eax          # get high 20 bits, not needed since it is 4kB align.
    movl    kernel_page_directory, %ecx  # index 0 of PD
    orl     %eax, %ecx
    movl    %ecx, kernel_page_directory

    # Enable paging by setting registers

    # set CR3 (PDBR). The high 20 bits of cr3 is the base address
    movl    $kernel_page_directory, %eax    # get the address of the PD
    # andl    $0xFFFFF000, %eax             # get high 20 bits, not necessary since it is 4kB align
    movl    %cr3, %ecx
    orl     %eax, %ecx
    movl    %ecx, %cr3

    # set CR4. Bit 4: PSE
    movl    %cr4, %eax
    orl     $0x10, %eax
    movl    %eax, %cr4

    # Set CR0. Bit 31: paging flag, bit 0: protection enabled
    movl    %cr0, %eax
    orl     $0x80000001, %eax
    movl    %eax, %cr0

    leave
    ret



