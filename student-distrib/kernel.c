/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"
#include "idt.h"
#include "file_system.h"
#include "rtc.h"
#include "terminal.h"
#include "task.h"
#include "modex.h"
#include "mouse.h"
#include "vga/vga.h"
#include "png/png.h"
#include "gui/gui.h"


static unsigned char png_test[MAX_PNG_SIZE];

#define RUN_TESTS

/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

void __sleep() {
    int i;
    for (i = 0; i < 10000; i++) {}
}

static uint8_t png_buffer[500 * 1024];

extern void enable_paging();  // in boot.S

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;
    set_mode_X();

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
        tss.esp0 = KERNEL_ESP_START;
        ltr(KERNEL_TSS);
    }

    /* Init the PIC */
    i8259_init();

    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */

    /* Init the keyboard */
    keyboard_init();
    enable_irq(KEYBOARD_IRQ_NUM);

    /* Init the mouse */
    mouse_init();
    enable_irq(MOUSE_IRQ_NUM);
    printf("mouse initialized!!!!!!!!");

    /* Init the RTC */
    rtc_init();
    enable_irq(RTC_IRQ_NUM);  // enable IRQ after setting up RTC
    rtc_restart_interrupt();  // in case that an interrupt happens after rtc_init() and before enable_irq

    /* Init the file system */
    if (mbi->mods_count == 0) {
        printf("WARNING: no file system loaded\n");
    } else {
        init_file_system((module_t *) mbi->mods_addr);
    }

    /* Enable interrupts */
    idt_init();

    /* Enable paging */
    enable_paging();

    /* Init the process system */
    task_init();

    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    //printf("Enabling Interrupts\n");
    sti();

    int i;

    vga_init();
    vga_set_mode(G1024x768x64K);
    vga_clear();

    gui_obj_load();

//    vga_screen_off();
//    {
//        for (i = 0; i < 1000; i++) {
//            vga_clear();
//        }
//    }
//    vga_screen_on();
//
//    vga_screen_off();
//    {
//        for (i = 0; i < 1000 * 1024 * 768; i++) {
//            vga_draw_pixel(0, 0);
//        }
//    }
//    vga_screen_on();

    int x, y;

//    vga_screen_off();
    {
        vga_set_color_argb(0xFFFF0000);
        for (x = 0; x < 300; x++) {
            for (y = 0; y <= 300; y++) {
                vga_draw_pixel(x, y);
//                __sleep();
            }
        }


//        for (i = 0; i < 1000; i++) {
//            vga_screen_copy(0, 0, 0, VGA_HEIGHT, VGA_WIDTH, VGA_HEIGHT);
//        }

        vga_set_color_argb(0xFF00FF00);
        for (x = 200; x < 500; x++) {
            for (y = 200; y <= 500; y++) {
                vga_draw_pixel(x, y);
            }
        }

        vga_set_color_argb(0xFF0000FF);
        for (x = 400; x < 700; x++) {
            for (y = 400; y <= 700; y++) {
                vga_draw_pixel(x, y);
            }
        }

        vga_set_color_argb(0xFFFFFF00);
        for (x = 730; x < 760; x++) {
            for (y = 30; y <= 60; y++) {
                vga_draw_pixel(x, y);
            }
        }

        vga_set_color_argb(0xFF00FFFF);
        for (x = 730; x < 760; x++) {
            for (y = 90; y <= 120; y++) {
                vga_draw_pixel(x, y);
            }
        }

        vga_set_color_argb(0xFFFF00FF);
        for (x = 730; x < 760; x++) {
            for (y = 150; y <= 180; y++) {
                vga_draw_pixel(x, y);
            }
        }

//        for (x = 0; x < 1024; x++) {
//                vga_set_color_argb(0 * 512 + x);
//                vga_draw_pixel(x, 1);
////                __sleep();
//        }

    }

//    while(1) {}

    /* Try loading png */
    dentry_t test_png;
    int32_t readin_size;
//    uint32_t file_length = 0xffffffff;

    // Open the png file and read it into buffer
    int32_t read_png = read_dentry_by_name("test.png", &test_png);
    if (0 == read_png) {
        readin_size = read_data(test_png.inode_num, 0, png_buffer, sizeof(png_buffer));
        if (readin_size == sizeof(png_buffer)) {
            DEBUG_ERR("PNG SIZE NOT ENOUGH!");
        }
    }

    upng_t upng;
    unsigned width;
    unsigned height;
    unsigned px_size;
    const unsigned char *buffer;
    vga_argb c;

    upng = upng_new_from_file(png_buffer, (long) readin_size);
    upng.buffer = (unsigned char *) &png_test;
    upng_decode(&upng);
    if (upng_get_error(&upng) == UPNG_EOK) {
        width = upng_get_width(&upng);
        height = upng_get_height(&upng);
        px_size = upng_get_pixelsize(&upng) / 8;
        printf("px_size = %u\n", px_size);
        buffer = upng_get_buffer(&upng);
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                c = 0;
                for (int d = 0; d < px_size; d++) {
                    c = (c << 8) | buffer[(j * width + i) * px_size + (px_size - d - 1)];
                }
//                printf("%x ", buffer[(j * width + i) * px_size + d]);
                vga_set_color_argb(c | 0xFF000000);
                vga_draw_pixel(i, j);
            }
        }
    }


    while (1) {}
//    vga_screen_on();

    vga_screen_copy(0, 0, 600, 600, 100, 100);
//    vga_screen_copy(1, 100, 0, 0, 1024, 600);




//    while (1) {}

    /* Run tests */

    launch_tests();
    /* Execute the first program ("shell") ... */

    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}
