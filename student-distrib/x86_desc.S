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

# PDs and PTs
.globl kernel_page_directory
.globl kernel_page_table
.globl kernel_page_table_0
.globl kernel_page_table_1
.globl user_page_table_0
.globl kernel_vidmem_buffer

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

    # Index 0: 0-4MB, map to a page table, in boot.S
    .long   0x00000003  # the 3 is for present and W/R flag

    # Index 1: 4-8MB, map to physical memory 4-8MB
    .long   0x00400083  # the 3 is for present and W/R flag, 8 is for PS flag, 4 for 4MB place

    # kernel offsets: hard coded: should changed according to KERNEL_PAGE_OFFSET
    .long   0x00800083
    .long   0x00C00083
    .long   0x01000083
    .long   0x01400083
    .long   0x01800083

    kernel_vidmem_buffer:
    .long   0x00000003  # set in boot.S

    # remaining: not presented
    .rept KERNEL_PAGE_DIRECTORY_SIZE - KERNEL_PAGE_OFFSET - 3
    .long 0
    .endr


.align SIZE_4K  # Align to 4 kB
kernel_page_table:
kernel_page_table_0:
    # Fill index 0-0xB7
    .rept 0xA0   # modex
    .long 0
    .endr

    # video memory, modex
    .long   0x000A0003
    .long   0x000A1003
    .long   0x000A2003
    .long   0x000A3003
    .long   0x000A4003
    .long   0x000A5003
    .long   0x000A6003
    .long   0x000A7003
    .long   0x000A8003
    .long   0x000A9003
    .long   0x000AA003
    .long   0x000AB003
    .long   0x000AC003
    .long   0x000AD003
    .long   0x000AE003
    .long   0x000AF003
    .long   0x000B0003
    .long   0x000B1003
    .long   0x000B2003
    .long   0x000B3003
    .long   0x000B4003
    .long   0x000B5003
    .long   0x000B6003
    .long   0x000B7003
    .long   0x000B8003
    .long   0x000B9003
    .long   0x000BA003
    .long   0x000BB003
    .long   0x000BC003
    .long   0x000BD003
    .long   0x000BE003
    .long   0x000BF003


    # Fill the rest to 1024
    .rept KERNEL_PAGE_TABLE_SIZE - 0xA0 - 32   # modex
    .long 0
    .endr

kernel_page_table_1: # the page that can access all buffer, for PDE VRAM_BUFFER_PD_ENTRY * 4MB ~ +1
    # Filled in vidmem.c init
    .rept SIZE_K
    .long 0
    .endr

user_page_table_0:
    # Fill index 0-0xB7
    .rept VIDEO_MEMORY_START_PAGE   # 0xB8
    .long 0
    .endr

    # 0xB8: video memory
    .long   0x000B8007  # the 7 is for U, present and W/R flag

    # Fill the rest to 1024
    .rept KERNEL_PAGE_TABLE_SIZE - VIDEO_MEMORY_START_PAGE - 1   # 1024 - 0xB8 -1
    .long 0
    .endr

