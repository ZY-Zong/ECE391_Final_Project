#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "file_system.h"

#define FD_STDIN     0
#define FD_STDOUT    1

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER    \
    printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)    \
    printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL")

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
        debug_err("CR3 not correct");
        result = FAIL;
    }

    // Check CR0
    asm volatile ("movl %%cr0, %0"
    : "=r" (tmp)
    :
    : "memory", "cc");
    if ((tmp & 0x80000000) == 0) {
        printf(PRINT_ERR"Paging is not enabled");
        result = FAIL;
    }

    // Check kernel page directory
    if ((kernel_page_table_t *) (kernel_page_directory.entry[0] & 0xFFFFF000) != &kernel_page_table_0) {
        printf(PRINT_ERR"Paging directory entry 0 is not correct");
        result = FAIL;
    }
    if ((kernel_page_directory.entry[1] & 0x00400000) != 0x00400000) {
        printf(PRINT_ERR"Paging directory entry 1 is not correct");
        result = FAIL;
    }

    // Check kernel page table 0
    for (i = 0; i < VIDEO_MEMORY_START_PAGE; ++i) {
        if (kernel_page_table_0.entry[i] != 0) {
            printf(PRINT_ERR"Paging table entry %d is not correct", i);
            result = FAIL;
        }
    }

    // Check video memory configuration
    if ((kernel_page_table_0.entry[VIDEO_MEMORY_START_PAGE] & 0x000B8003) != 0x000B8003) {
        printf("Paging table entry for video memory is not correct");
        result = FAIL;
    }
    for (i = VIDEO_MEMORY_START_PAGE + 1; i < KERNEL_PAGE_TABLE_SIZE; ++i) {
        if (kernel_page_table_0.entry[i] != 0) {
            printf(PRINT_ERR"Paging table entry %d is not correct", i);
            result = FAIL;
        }
    }

    // Try to dereference some variables
    int *i_ptr = &i;
    if (*i_ptr != i) {
        printf(PRINT_ERR"Dereference i error");
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
    printf("j = %u", j);

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
    printf("j = %u", j);

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
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests() {

    // Clear screen
    clear();
    reset_cursor();

    TEST_OUTPUT("idt_test", idt_test());
    TEST_OUTPUT("paging_test", paging_test());
    TEST_OUTPUT("rtc_test", rtc_test());
    press_enter_to_continue();
    TEST_OUTPUT("terminal_test", terminal_test());
    press_enter_to_continue();
    TEST_OUTPUT("fs_test", fs_test());

    printf("\nTests complete.\n");
}
