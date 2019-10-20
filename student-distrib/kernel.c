/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"

#define RUN_TESTS

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

/** Keyboard related constants */
#define KEYBOARD_IRQ_NUM   1
#define KEYBOARD_PORT   0x60    /* keyboard scancode port */

/* Keys that correspond to scan codes, using scan code set 1 for "US QWERTY" keyboard
 * REFERENCE: https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Sets.2C_Scan_Codes_and_Key_Codes
 */
static const char scan_code_table[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,      /* 0x00 - 0x0E */
        0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'o', 'p', '[', ']', '\n',           /* 0x0F - 0x1C */
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',           /* 0x1D - 0x29 */
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0,          /* 0x2A - 0x37 */
        0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                            /* 0x38 - 0x46 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                    /* 0x47 - 0x53 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                     /* 0x54 - 0x80 */
};

/** RTC related constants */
#define RTC_IRQ_NUM   8

/* RTC Status Registers */
#define RTC_STATUS_REGISTER_A   0x0A
#define RTC_STATUS_REGISTER_B   0x0B
#define RTC_STATUS_REGISTER_C   0x0C

/* 2 IO ports used for the RTC and CMOS */
#define RTC_REGISTER_PORT       0x70
#define RTC_RW_DATA_PORT        0x71

#define RTC_DEFAULT_FREQUENCY   1024


#define IDT_ENTRY_INTEL          0x20  // number of vectors used by intel
#define IDT_ENTRY_KEYBOARD       0x21  // the vector number of keyboard
#define IDT_ENTRY_RTC            0x28  // the vector number of RTC
#define IDT_ENTRY_SYSTEM_CALL    0x80  // the vector number of system calls

void idt_init();

void rtc_init();

void keyboard_interrupt_handler();

void rtc_interrupt_handler();

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;

    /* Clear the screen. */
    clear();

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned) magic);
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned) mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *) mbi->cmdline);

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t *mod = (module_t *) mbi->mods_addr;
        while (mod_count < mbi->mods_count) {
            printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int) mod->mod_start);
            printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int) mod->mod_end);
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char *) (mod->mod_start + i)));
            }
            printf("\n");
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
        printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
               (unsigned) elf_sec->num, (unsigned) elf_sec->size,
               (unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
               (unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
        for (mmap = (memory_map_t *) mbi->mmap_addr;
             (unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
             mmap = (memory_map_t *) ((unsigned long) mmap + mmap->size + sizeof(mmap->size)))
            printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
                   (unsigned) mmap->size,
                   (unsigned) mmap->base_addr_high,
                   (unsigned) mmap->base_addr_low,
                   (unsigned) mmap->type,
                   (unsigned) mmap->length_high,
                   (unsigned) mmap->length_low);
    }

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize = 0x1;
        the_ldt_desc.reserved = 0x0;
        the_ldt_desc.avail = 0x0;
        the_ldt_desc.present = 0x1;
        the_ldt_desc.dpl = 0x0;
        the_ldt_desc.sys = 0x0;
        the_ldt_desc.type = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity = 0x0;
        the_tss_desc.opsize = 0x0;
        the_tss_desc.reserved = 0x0;
        the_tss_desc.avail = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present = 0x1;
        the_tss_desc.dpl = 0x0;
        the_tss_desc.sys = 0x0;
        the_tss_desc.type = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        ltr(KERNEL_TSS);
    }

    /* Init the PIC */
    i8259_init();

    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */

    /* Init the keyboard */
    enable_irq(KEYBOARD_IRQ_NUM);

    /* Init the RTC */
    rtc_init();
    enable_irq(RTC_IRQ_NUM);

    /* Enable interrupts */
    idt_init();

    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    /*printf("Enabling Interrupts\n");
    sti();*/

#ifdef RUN_TESTS
    /* Run tests */
    launch_tests();
#endif
    /* Execute the first program ("shell") ... */

    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}

/*
 *************** Caution *****************:
 * This version only initialize the idt table, leaving out the first 32 exceptions defined by Intel.
 * Also, the interrupt handler pointer for keyboard and RTC is not set.
 * System call pointer also is not set.
 */

/*
 * idt_init
 * This function is used to initialize IDT table and called in kernel.c.
 * Uses subroutine provided in x86_desc.h.
 * Input: None.
 * Output: None.
 * Side effect: Change the IDT table defined in x86_desc.S.
 */
extern void idt_init() {

    int i;

    // Initialize 0x00 - 0x1F exception handler defined by Intel.
    for (i = 0; i < IDT_ENTRY_INTEL; i++) {
        idt[i].seg_selector = KERNEL_CS;
        idt[i].dpl = 0;
    }
    // Initialize 0x20 - 0xFF general purpose interrupt handlers
    for (i = IDT_ENTRY_INTEL; i < NUM_VEC; i++) {
        idt[i].seg_selector = KERNEL_CS;
        idt[i].dpl = 0;
    }
    // Set keyboard handler
    SET_IDT_ENTRY(idt[IDT_ENTRY_KEYBOARD], keyboard_interrupt_handler);

    // Set RTC handler
    SET_IDT_ENTRY(idt[IDT_ENTRY_RTC], rtc_interrupt_handler);

    // Initialize system calls
    // TODO: After writing the RTC handler, uncomment and fill in the pointer
    // SET_IDT_ENTRY(idt[IDT_ENTRY_SYSTEM_CALL], );
    idt[IDT_ENTRY_SYSTEM_CALL].dpl = 3;
}

/*
 * keyboard_interrupt
 *   REFERENCE: https://wiki.osdev.org/PS2_Keyboard#Scan_Code_Sets.2C_Scan_Codes_and_Key_Codes
 *   DESCRIPTION: Handle the keyboard interrupts
 *   INPUTS: scan code from port 0x60
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: interrupt if necessary
 */
void keyboard_interrupt_handler() {

    cli();
    {
        /* Get scan code from port 0x60 */
        uint8_t scancode = inb(KEYBOARD_PORT);

        /* Output the char to the console */
        putc(scan_code_table[scancode]);

    }
    sti();

    /* Send EOI */
    // TODO: use a function to handle all EOF
    send_eoi(KEYBOARD_IRQ_NUM);
}

/*
 * rtc_init
 *   REFERENCE: https://wiki.osdev.org/RTC
 *   DESCRIPTION: Initialize the real time clock
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Frequency is set to be default 1024 Hz
 */
void rtc_init() {
    /* Turn on IRQ 8, default frequency is 1024 Hz */

    cli();  // disable interrupts
    {
        outb(RTC_STATUS_REGISTER_B, RTC_REGISTER_PORT);  // select register B and disable NMI
        char prev = inb(RTC_RW_DATA_PORT);    // read the current value of register B
        outb(RTC_STATUS_REGISTER_B,
             RTC_REGISTER_PORT);     // set the index again (a read will reset the index to register D)
        outb (prev | 0x40,
              RTC_RW_DATA_PORT);     // write the previous value ORed with 0x40. This turns on bit 6 of register B
    }
    sti();
}

/*
 * rtc_interrupt_handler
 *   REFERENCE: https://wiki.osdev.org/RTC
 *   DESCRIPTION: Handle the RTC interrupt. Note that Status Register C will contain a bitmask
 *                telling which interrupt happened. If register C is not read after an IRQ 8,
 *                then the interrupt will not happen again.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void rtc_interrupt_handler() {

    /* Get another interrupt */
    outb(RTC_STATUS_REGISTER_C, RTC_REGISTER_PORT);    // select register C
    inb(RTC_RW_DATA_PORT); // just throw away contents

    printf("Receive an RTC interrupt");

    /* Send EOI */
    send_eoi(RTC_IRQ_NUM);
}