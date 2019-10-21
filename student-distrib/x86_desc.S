# x86_desc.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#include "x86_desc.h"

.text

.globl ldt_size, tss_size
.globl gdt_desc, ldt_desc, tss_desc
.globl tss, tss_desc_ptr, ldt, ldt_desc_ptr
.globl gdt_ptr
.globl gdt_desc_ptr
.globl idt_desc_ptr, idt

# mp3.1 page table
.globl kernel_page_directory
.globl kernel_page_table
.globl kernel_page_table_0

.align 4


tss_size:
    .long tss_bottom - tss - 1

ldt_size:
    .long ldt_bottom - ldt - 1

    .word 0 # Padding
ldt_desc:
    .word KERNEL_LDT
    .long ldt

    .align 4
tss:
_tss:
    .rept 104
    .byte 0
    .endr
tss_bottom:

# Data structure used to load GDT
# Low 2 bytes are (size - 1)
# High 4 bytes are the pointer to GDT in x86_desc.S
    .align 4
gdt_desc_ptr:
    .word gdt_bottom - gdt - 1
    .long gdt

    .align  16
gdt:
_gdt:

    # First GDT entry cannot be used
    .quad 0

    # NULL entry
    .quad 0

    # Segmentation will not be used
    # CS and DS both are 0-4GB r/w segments
    #
    # The layout is (from Intel IA-32 reference manual):
    #  31        24 23  22  21  20  19   16 15  14 13 12  11   8 7          0
    # |----------------------------------------------------------------------|
    # |            |   | D |   | A |  Seg  |   |  D  |   |      |            |
    # | Base 31:24 | G | / | 0 | V | Limit | P |  P  | S | Type | Base 23:16 |
    # |            |   | B |   | L | 19:16 |   |  L  |   |      |            |
    # |----------------------------------------------------------------------|
    #
    # |----------------------------------------------------------------------|
    # |                                    |                                 |
    # | Base 15:0                          | Segment Limit 15:0              |
    # |                                    |                                 |
    # |----------------------------------------------------------------------|

gdt_ptr:
    # Set up an entry for kernel CS
    .quad 0x00CF9A000000FFFF

    # Set up an entry for kernel DS
    .quad 0x00CF92000000FFFF

    # Set up an entry for user CS
    .quad 0x00CFFA000000FFFF

    # Set up an entry for user DS
    .quad 0x00CFF2000000FFFF

    # Set up an entry for TSS
tss_desc_ptr:
    .quad 0

    # Set up one LDT
ldt_desc_ptr:
    .quad 0

gdt_bottom:

    .align 16
ldt:
    .rept 4
    .quad 0
    .endr
ldt_bottom:

    .align 4
    .word 0 # Padding
idt_desc_ptr:
    .word idt_bottom - idt - 1
    .long idt


    .align  16
idt:
_idt:
    .rept NUM_VEC
    .quad 0
    .endr

idt_bottom:


.align SIZE_4K  # Align to 4 kB
kernel_page_directory:

    # Index 0: 0-4MB, map to a page table, set above
    .long   0x00000003  # the 3 is for present and W/R flag

    # Index 1: 4-8MB, map to physical memory 4-8MB
    .long   0x00400083  # the 3 is for present and W/R flag, 8 is for PS flag, 4 for 1MB place

    # Index 2-1023: not presented
    .rept KERNEL_PAGE_DIRECTORY_SIZE - 2  # 1024 - 2
    .long 0
    .endr


.align SIZE_4K  # Align to 4 kB
kernel_page_table:
kernel_page_table_0:
    # Fill index 0-0xB7
    .rept VIDEO_MEMORY_START_PAGE   # 0xB8
    .long 0
    .endr

    # 0xB8: video memory
    .long   0x000B8003  # the 3 is for present and W/R flag

    # Fill the rest to 1024
    .rept KERNEL_PAGE_TABLE_SIZE - VIDEO_MEMORY_START_PAGE - 1   # 1024 - 0xB8 -1
    .long 0
    .endr

