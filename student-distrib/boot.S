# boot.S - start point for the kernel after GRUB gives us control
# vim:ts=4 noexpandtab

#define ASM     1
#define ENABLE_PAGING 0

#include "multiboot.h"
#include "x86_desc.h"
#include "paging.h"

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


    #if ENABLE_PAGING
        # enable paging 
        /*
        * paging flag: bit 31 of CR0
        * protection enable: bit 0 of CR0
        */   
        movl    %cr0, %eax
        orl     $0x80000001, %eax  
        movl    %eax, %cr0;   
        
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

        # fill the PD and PT 
        # call    init_paging     already done in version 1.1

        # check whether the init success, not needed in version 1.1
        #cmpl    $0, %eax 
        #jne     paging_init_fail

    #endif /* ENABLE_PAGING */


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


# #if ENABLE_PAGING
    
    #paging_init_fail:
        # report the exception 
        # not needed in verison 1.1 

# #endif /* ENABLE_PAGING */

