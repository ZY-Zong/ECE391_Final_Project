#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "file_system.h"
#include "task/task.h"
#include "vga/vga.h"
#include "gui/gui.h"
#include "gui/upng.h"

#define FD_STDIN     0
#define FD_STDOUT    1

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER    \
    printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)    \
    printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL")

/** Test Printing Macros */
// Use printf() if things to print are normal effect of demonstration.
// Only use the following macros when info are optional.
#define TEST_PRINT_VERBOSE    0
#if TEST_PRINT_VERBOSE
#define TEST_PRINT(fmt, ...)    do { printf("%s:%d:%s(): " fmt, \
                                             __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define TEST_ERR(fmt, ...)      do { printf("[ERROR]" "%s:%d:%s(): " fmt, \
                                             __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define TEST_WARN(fmt, ...)     do { printf("[WARNING]" "%s:%d:%s(): " fmt, \
                                             __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#else
#define TEST_PRINT(fmt, ...)    do { printf(fmt, ##__VA_ARGS__); } while (0)
#define TEST_ERR(fmt, ...)      do { printf("[ERROR]" fmt, ##__VA_ARGS__); } while (0)
#define TEST_WARN(fmt, ...)     do { printf("[WARNING]" fmt, ##__VA_ARGS__); } while (0)
#endif

void __sleep() {
    int i;
    for (i = 0; i < 10000; i++) {}
}

static inline void assertion_failure() {
    /* Use exception #15 for assertions, otherwise
       reserved by Intel */
    asm volatile("int $15");
}

void press_enter_to_continue() {

    uint8_t buf[256];

    printf("Press enter to continue...");
    (void) read(FD_STDIN, &buf, 256);

    // Clear screen
    clear();
    reset_cursor();
}

/* Checkpoint 1 tests */

/**
 * Asserts that first 10 IDT entries are not NULL
 * @return PASS/FAIL
 */
long idt_test() {
    TEST_HEADER;

    long i;
    long result = PASS;
    for (i = 0; i < 10; ++i) {
        if ((idt[i].offset_15_00 == NULL) &&
            (idt[i].offset_31_16 == NULL)) {
            assertion_failure();
            result = FAIL;
        }
    }

    return result;
}

/**
 * Test paging data structure
 * @return PASS or FAIL
 */
int paging_test() {
    TEST_HEADER;

    int result = PASS;
    int i;
    unsigned long tmp;

    // Check CR3 content
    asm volatile ("movl %%cr3, %0"
    : "=r" (tmp)
    :
    : "memory", "cc");
    if ((kernel_page_directory_t *) tmp != &kernel_page_directory) {
        TEST_ERR("CR3 not correct");
        result = FAIL;
    }

    // Check CR0
    asm volatile ("movl %%cr0, %0"
    : "=r" (tmp)
    :
    : "memory", "cc");
    if ((tmp & 0x80000000) == 0) {
        TEST_ERR("Paging is not enabled");
        result = FAIL;
    }

    // Check kernel page directory
    if ((page_table_t *) (kernel_page_directory.entry[0] & 0xFFFFF000) != &kernel_page_table_0) {
        TEST_ERR("Paging directory entry 0 is not correct");
        result = FAIL;
    }
    if ((kernel_page_directory.entry[1] & 0x00400000) != 0x00400000) {
        TEST_ERR("Paging directory entry 1 is not correct");
        result = FAIL;
    }

    // Check kernel page table 0
    for (i = 0; i < VIDEO_MEMORY_START_PAGE; ++i) {
        if (kernel_page_table_0.entry[i] != 0) {
            TEST_ERR("Paging table entry %d is not correct", i);
            result = FAIL;
        }
    }

    // Check video memory configuration
    if ((kernel_page_table_0.entry[VIDEO_MEMORY_START_PAGE] & 0x000B8003) != 0x000B8003) {
        TEST_ERR("Paging table entry for video memory is not correct");
        result = FAIL;
    }
    for (i = VIDEO_MEMORY_START_PAGE + 1; i < KERNEL_PAGE_TABLE_SIZE; ++i) {
        if (kernel_page_table_0.entry[i] != 0) {
            TEST_ERR("Paging table entry %d is not correct", i);
            result = FAIL;
        }
    }

    // Try to dereference some variables
    int *i_ptr = &i;
    if (*i_ptr != i) {
        TEST_ERR("Dereference i error");
        result = FAIL;
    }

    return result;
}

/**
 * Try to divide 0. Cause Divide Error exception.
 * @return Should not return. Is so, FAIL
 */
long divide_zero_test() {
    TEST_HEADER;

    // Set i to 0, but avoid compile warnings
    unsigned long i = 42;
    i -= i;

    // Divide 0
    unsigned long j = 42 / i;

    // To avoid compiler warnings. Should not get here
    TEST_ERR("j = %u", j);

    return FAIL;
}

/**
 * Try to dereference Null. Cause Page Fault if paging is turning on.
 * @return Should not return. Is so, FAIL
 */
long dereference_null_test() {
    TEST_HEADER;

    // Set i to 0, but avoid compile warnings
    unsigned long i = 42;
    i -= i;

    // Dereference NULL
    unsigned long j = *((unsigned long *) i);

    // To avoid compiler warnings. Should not get here
    TEST_ERR("j = %u", j);

    return FAIL;
}

/* Checkpoint 2 tests */

/**
 * Test RTC read/write.
 * @return PASS or FAIL
 */
long rtc_test() {
    TEST_HEADER;

    long result = PASS;

    unsigned long fd;
    unsigned long freq;
    long ret;

    const unsigned long valid_freq[] = {2, 4, 16, 128, 1024};
    const unsigned long base_test_count = 3;
    const unsigned long invalid_freq[] = {3, 42, 8192};

    unsigned long i;
    unsigned long j;

    fd = open((uint8_t *) "rtc");

    // Read from 2 Hz RTC
    printf("Waiting for 2Hz RTC...");
    if (0 == (ret = read(fd, NULL, 0))) {
        printf("Done\n");
    } else {
        printf("Fail with return code %d\n", ret);
        result = FAIL;
    }

    // Set and read RTC for valid frequencies
    for (i = 0; i < sizeof(valid_freq) / sizeof(unsigned long); i++) {
        // Set RTC
        freq = valid_freq[i];
        printf("Setting RTC to %uHz...", freq);
        if (0 == (ret = write(fd, &freq, 4))) {
            printf("Done\n");
        } else {
            printf("Fail with return code %d\n", ret);
            result = FAIL;
        }

        // Read RTC
        printf("Waiting for %uHz RTC", freq);
        for (j = 0; j < base_test_count * valid_freq[i]; j++) {
            if (0 == (ret = read(fd, NULL, 0))) {
                printf(".");
            } else {
                break;
            }
        }
        if (j == base_test_count * valid_freq[i]) {
            printf("Done\n");
        } else {
            printf("Fail with return code %d\n", ret);
            result = FAIL;
        }

    }

    // Set and read RTC for invalid frequencies
    for (i = 0; i < sizeof(invalid_freq) / sizeof(unsigned long); i++) {
        // Set RTC
        freq = invalid_freq[i];
        printf("Try setting RTC to %uHz...", freq);
        if (-1 == (ret = write(fd, &freq, 4))) {
            printf("Correct\n");
        } else {
            printf("Incorrect return code %d\n", ret);
            result = FAIL;
        }
    }

    close(fd);

    return result;
}

/**
 * Test terminal read/write. Required user interaction.
 * @return PASS or FAIL
 */
long terminal_test() {
    TEST_HEADER;

    long result = PASS;
    char buf[256];
    unsigned long ret;
    unsigned long i;

    int32_t valid_write_size[] = {1, 16, 64, 128};
    int32_t invalid_write_size[] = {};

    // Test reading
    for (i = 1; i <= 3; i++) {
        printf("Reading from terminal (%u/3) ...\n", i);
        if (-1 == (ret = read(FD_STDIN, buf, 255))) {
            printf("Fail\n");
            result = FAIL;
        } else {
            buf[ret] = '\0';
            printf("... Done with size %d, the content is:\n%s\n", ret, buf);
        }
    }

    // Fill buf[] with characters
    for (i = 0; i < 255; i++) {
        buf[i] = '-';
    }
    buf[255] = '\0';

    // Test valid write
    for (i = 0; i < sizeof(valid_write_size) / sizeof(int32_t); i++) {
        printf("Writing to terminal of size %u ...\n", valid_write_size[i]);
        if (valid_write_size[i] == (ret = write(FD_STDOUT, buf, valid_write_size[i]))) {
            printf("... Done\n");
        } else {
            printf("... Fail with return %d\n", ret);
            result = FAIL;
        }
    }

    // Test invalid write
    for (i = 0; i < sizeof(invalid_write_size) / sizeof(int32_t); i++) {
        printf("Try writing to terminal of size %u ...\n", invalid_write_size[i]);
        if (128 == (ret = write(FD_STDOUT, buf, invalid_write_size[i]))) {
            printf("... Correct\n");
        } else {
            printf("... Incorrect with return %d\n", ret);
            result = FAIL;
        }
    }

    return result;
}

/**
 * Test file system
 * @return PASS or FAIL
 */
long fs_test() {
    TEST_HEADER;

    long result = PASS;

    unsigned long fd;
    const size_t buf_size = 33;
    char buf[buf_size];
    unsigned long ret;

    const char *valid_test_file[] = {"sigtest", "shell", "grep", "syserr", "rtc", "fish", "frame0.txt", "frame1.txt",
                                     "created.txt", "verylargetextwithverylongname.tx"};
    const char *invalid_test_file[] = {"non_exist_file"};

    unsigned long i;

    // Read root directory
    if (-1 == (fd = open((uint8_t *) "."))) {
        printf("Failed to open \".\"\n");
        result = FAIL;
    } else {
        while (0 != (ret = read(fd, buf, buf_size - 1))) {
            if (-1 == ret) {
                printf("Failed to read \".\"\n");
                return FAIL;
            }
            // buf[ret] = '\n';
            if (-1 == write(FD_STDOUT, buf, ret )) { // +1 if need \n here 
                printf("Failed to write to stdout\n");
                return FAIL;
            }
            // Print out the file type and size  
            dentry_t cur_file;
            if ( -1 == read_dentry_by_name((uint8_t*)buf, &cur_file)){
                printf("buf didn't contain correct file name\n");
            }
            if (cur_file.file_type != 2 ){
                printf(" file type: %d\n", cur_file.file_type);
            } else{
                printf(" file type: %d, file size: %dB\n", 
                cur_file.file_type, get_file_size(cur_file.inode_num));
            }
            
        }
    }

    close(fd);

    press_enter_to_continue();

    // Test reading files
    for (i = 0; i < sizeof(valid_test_file) / sizeof(const char *); i++) {

        clear();
        reset_cursor();

        if (-1 == (fd = open((const uint8_t *) valid_test_file[i]))) {
            printf("Failed to open %s\n", valid_test_file[i]);
            result = FAIL;
        } else {
            while (0 != (ret = read(fd, buf, buf_size - 1))) {
                if (-1 == ret) {
                    printf("Failed to read %s\n", valid_test_file[i]);
                    close(fd);
                    result = FAIL;
                }
                buf[ret] = '\0';
                if (-1 == write(FD_STDOUT, buf, ret)) {
                    printf("Failed to write to stdout\n");
                    close(fd);
                    result = FAIL;
                }
            }
        }

        close(fd);

        printf("\nThis is content of file %s (%u/%u)\n", valid_test_file[i], i + 1,
                sizeof(valid_test_file) / sizeof(const char *));

        press_enter_to_continue();
    }

    // Test invalid files
    for (i = 0; i < sizeof(invalid_test_file) / sizeof(const char *); i++) {

        clear();
        reset_cursor();

        printf("Try reading %s (%u/%u)...", invalid_test_file[i], i + 1,
                sizeof(invalid_test_file) / sizeof(const char *));

        if (-1 == (fd = open((const uint8_t *) invalid_test_file[i]))) {
            printf("Correct\n");
        } else {
            printf("Incorrect with fd = %u\n", fd);
            close(fd);
            result = FAIL;
        }

        press_enter_to_continue();
    }

    return result;

}

/* Checkpoint 3 tests */

/**
 * Test error arguments to fs system calls
 * @return PASS or FAIL
 * @note Inspired by ece391syserr.c
 */
long fs_err_test() {
    TEST_HEADER;

    long result = PASS;
    int32_t ret;
    uint8_t buf[32];

    printf("Try passing error fds to fs syscalls...\n");
    if (-1 != (ret = read(-1, buf, 31))) {
        printf("read error return value %d\n", ret);
        result = FAIL;
    }
    if (-1 != (ret = read(99999999, buf, 31))) {
        printf("read error return value %d\n", ret);
        result = FAIL;
    }
    if (-1 != (ret = write(-1, buf, 31))) {
        printf("write error return value %d\n", ret);
        result = FAIL;
    }
    if (-1 != (ret = write(99999999, buf, 31))) {
        printf("write error return value %d\n", ret);
        result = FAIL;
    }
    if (-1 != (ret = close(-1))) {
        printf("close error return value %d\n", ret);
        result = FAIL;
    }
    if (-1 != (ret = close(99999999))) {
        printf("close error return value %d\n", ret);
        result = FAIL;
    }

    return result;
}

/**
 * Test error arguments to execute
 * @return PASS or FAIL
 */
long execute_err_test() {
    TEST_HEADER;

    long result = PASS;
    int32_t ret;
    uint32_t flags;

    printf("Try executing non-exist file...");

    cli_and_save(flags);
    {
        if (-1 != (ret = system_execute((uint8_t *) "non_exist", 0, 0, NULL))) {
            printf("error return value %d for non-exist file\n", ret);
            result = FAIL;
        } else {
            printf("Correct\n");
        }

        printf("Try executing non-executable file...");
        if (-1 != (ret = system_execute((uint8_t *) "frame1.txt", 0, 0, NULL))) {
            printf("error return value %d for non-executable file\n", ret);
            result = FAIL;
        } else {
            printf("Correct\n");
        }
    }
    restore_flags(flags);

    return result;
}

/**
 * Test whether all files are closed correctly. Used at halt().
 */
void checkpoint_task_closed_all_files() {
    int cnt = 0;
    int i;
    for (i = 0; i < MAX_OPEN_FILE; i++) {
        if (-1 != close(i)) {
            cnt++;  // count unclosed files
        }
    }
    if (cnt == 0) {
        printf("[PASS] system_halt(): closed all files.\n");
    } else {
        printf("[ERROR] system_halt(): didn't close all files!\n");
    }
}

/**
 * Test whether paging is consistent for current process
 */
void checkpoint_task_paging_consistent() {
    // check 128M - 132M maps to current process correctly

    uint32_t user_paging_idx = ((kernel_page_directory.entry[32] & 0xFFC00000) >> 22) - 2;
    uint32_t pcb_idx = (PKM_STARTING_ADDR - (uint32_t) running_task()) / PKM_SIZE_IN_BYTES - 1;

    if (user_paging_idx == pcb_idx) {
        printf("[PASS] system_execute/halt(): user paging consistent.\n");
    } else {
        printf("[ERROR] system_execute/halt(): user paging %u inconsistent with pcb idx %u.\n",
               user_paging_idx, pcb_idx);
    }
}

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

void svga_test() {

    int x, y;

    vga_set_color_argb(0xFFFF0000);
    for (x = 800; x < 830; x++) {
        for (y = 50; y <= 80; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_set_color_argb(0xFF00FF00);
    for (x = 800; x < 830; x++) {
        for (y = 110; y <= 140; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_set_color_argb(0xFF0000FF);
    for (x = 800; x < 830; x++) {
        for (y = 170; y <= 200; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_set_color_argb(0xFFFFFF00);
    for (x = 860; x < 890; x++) {
        for (y = 50; y <= 80; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_set_color_argb(0xFF00FFFF);
    for (x = 860; x < 890; x++) {
        for (y = 110; y <= 140; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_set_color_argb(0xFFFF00FF);
    for (x = 860; x < 890; x++) {
        for (y = 170; y <= 200; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_set_color_argb(0xAAFFFF00);
    for (x = 700; x < 900; x++) {
        for (y = 250; y <= 450; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_set_color_argb(0xAA00FFFF);
    for (x = 800; x < 1000; x++) {
        for (y = 350; y <= 550; y++) {
            vga_draw_pixel(x, y);
        }
    }

    vga_screen_copy(750, 400, 800, 600, 150, 150);
}
//
//static unsigned char png_file_buf[MAX_PNG_SIZE];
//static unsigned char png_test_buf[MAX_PNG_SIZE];
//
//void png_test() {
//
//    dentry_t test_png;
//    int32_t readin_size;
//
//    // Open the png file and read it into buffer
//    int32_t read_png = read_dentry_by_name("test.png", &test_png);
//    if (0 == read_png) {
//        readin_size = read_data(test_png.inode_num, 0, png_file_buf, sizeof(png_file_buf));
//        if (readin_size == sizeof(png_test_buf)) {
//            DEBUG_ERR("PNG SIZE NOT ENOUGH!");
//        }
//    }
//
//    upng_t upng;
//    unsigned width;
//    unsigned height;
//    unsigned px_size;
//    const unsigned char *buffer;
//    vga_argb c;
//
//    upng = upng_new_from_file(png_file_buf, (long) readin_size);
//    upng.buffer = (unsigned char *) &png_test_buf;
//    upng_decode(&upng);
//    if (upng_get_error(&upng) == UPNG_EOK) {
//        width = upng_get_width(&upng);
//        height = upng_get_height(&upng);
//        px_size = upng_get_pixelsize(&upng) / 8;
//        printf("px_size = %u\n", px_size);
//        buffer = upng_get_buffer(&upng);
//        for (int j = 0; j < height; j++) {
//            for (int i = 0; i < width; i++) {
//                c = 0;
//                for (int d = 0; d < px_size; d++) {
//                    c = (c << 8) | buffer[(j * width + i) * px_size + (px_size - d - 1)];
//                }
//                vga_set_color_argb(c | 0xFF000000);
//                vga_draw_pixel(i, j);
//            }
//        }
//    }
//}
//
//void png_alpha_test() {
//
//    dentry_t test_png;
//    int32_t readin_size;
//
//    // Open the png file and read it into buffer
//    int32_t read_png = read_dentry_by_name("alpha.png", &test_png);
//    if (0 == read_png) {
//        readin_size = read_data(test_png.inode_num, 0, png_file_buf, sizeof(png_file_buf));
//        if (readin_size == sizeof(png_test_buf)) {
//            DEBUG_ERR("PNG SIZE NOT ENOUGH!");
//        }
//    }
//
//    upng_t upng;
//    unsigned width;
//    unsigned height;
//    unsigned px_size;
//    const unsigned char *buffer;
//    vga_argb c;
//
//    upng = upng_new_from_file(png_file_buf, (long) readin_size);
//    upng.buffer = (unsigned char *) &png_test_buf;
//    upng_decode(&upng);
//    if (upng_get_error(&upng) == UPNG_EOK) {
//        width = upng_get_width(&upng);
//        height = upng_get_height(&upng);
//        px_size = upng_get_pixelsize(&upng) / 8;
//        printf("px_size = %u\n", px_size);
//        buffer = upng_get_buffer(&upng);
//        for (int j = 0; j < height; j++) {
//            for (int i = 0; i < width; i++) {
//                c = 0;
//                for (int d = 0; d < px_size; d++) {
//                    c = (c << 8) | buffer[(j * width + i) * px_size + (px_size - d - 1)];
//                }
//                vga_set_color_argb(c);
//                vga_draw_pixel(i, j);
//            }
//        }
//    }
//}
//
//void png_full_screen_test() {
//    dentry_t test_png;
//    int32_t readin_size;
//
//    // Open the png file and read it into buffer
//    int32_t read_png = read_dentry_by_name("fullscreen.png", &test_png);
//    if (0 == read_png) {
//        readin_size = read_data(test_png.inode_num, 0, png_file_buf, sizeof(png_file_buf));
//        if (readin_size == sizeof(png_test_buf)) {
//            DEBUG_ERR("PNG SIZE NOT ENOUGH!");
//        }
//    }
//
//    upng_t upng;
//    unsigned width;
//    unsigned height;
//    unsigned px_size;
//    const unsigned char *buffer;
//    vga_argb c;
//
//    upng = upng_new_from_file(png_file_buf, (long) readin_size);
//    upng.buffer = (unsigned char *) &png_test_buf;
//    upng_decode(&upng);
//    if (upng_get_error(&upng) == UPNG_EOK) {
//        width = upng_get_width(&upng);
//        height = upng_get_height(&upng);
//        px_size = upng_get_pixelsize(&upng) / 8;
//        printf("px_size = %u\n", px_size);
//        buffer = upng_get_buffer(&upng);
//        for (int j = 0; j < height; j++) {
//            for (int i = 0; i < width; i++) {
//                c = 0;
//                for (int d = 0; d < px_size; d++) {
//                    c = (c << 8) | buffer[(j * width + i) * px_size + (px_size - d - 1)];
//                }
//                vga_set_color_argb(c | 0xFF000000);
//                vga_draw_pixel(i, j);
//            }
//        }
//    }
//}
//

/* Test suite entry point */
void launch_tests() {

    // Clear screen
    clear();
    reset_cursor();

//    TEST_OUTPUT("idt_test", idt_test());
//    TEST_OUTPUT("paging_test", paging_test());
//    TEST_OUTPUT("rtc_test", rtc_test());
//    press_enter_to_continue();
//    TEST_OUTPUT("terminal_test", terminal_test());
//    press_enter_to_continue();
//    TEST_OUTPUT("fs_test", fs_test());
//    TEST_OUTPUT("fs_err_test", fs_err_test());
//    TEST_OUTPUT("execute_err_test", execute_err_test());
//    svga_test();
//    while(1) {}
//    png_test();
//    png_alpha_test();
//    png_full_screen_test();
    printf("\nTests complete.\n");
}
