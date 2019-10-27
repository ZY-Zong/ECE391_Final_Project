#include "tests.h"
#include "x86_desc.h"
#include "lib.h"

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
        printf(PRINT_ERR"CR3 not correct");
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
    const unsigned long test_count = 10;
    const unsigned long invalid_freq[] = {3, 42, 8192};

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
    for (unsigned long i = 0; i < sizeof(valid_freq); i++) {
        // Set RTC
        freq = valid_freq[i];
        printf("Setting RTC to %uHz...", freq);
        if (4 == (ret = write(fd, (char *) (&freq), 4))) {
            printf("Done\n");
        } else {
            printf("Fail with return code %d\n", ret);
            result = FAIL;
        }

        // Read RTC
        printf("Waiting for %uHz RTC", freq);
        unsigned long j;
        for (j = 0; j < test_count; j++) {
            if (0 == (ret = read(fd, NULL, 0))) {
                printf(".");
            } else {
                break;
            }
        }
        if (j == test_count) {
            printf("Done\n");
        } else {
            printf("Fail with return code %d\n", ret);
            result = FAIL;
        }

    }

    // Set and read RTC for invalid frequencies
    for (unsigned long i = 0; i < sizeof(invalid_freq); i++) {
        // Set RTC
        freq = invalid_freq[i];
        printf("Try setting RTC to %uHz...", freq);
        if (-1 == (ret = write(fd, (char *) (&freq), 4))) {
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

    size_t valid_write_size[] = {16, 128};
    size_t invalid_write_size[] = {200};

    // Test reading
    for (unsigned long i = 1; i <= 3; i++) {
        printf("Reading from terminal (%u/3) ...\n", i);
        if (-1 == (ret = read(FD_STDIN, buf, 255))) {
            printf("Fail\n");
            result = FAIL;
        } else {
            buf[ret] = '\0';
            printf("... Done with size %d\n, the content is:\n%s\n", ret, buf);
        }
    }

    // Fill buf[] with characters
    for (unsigned long i = 0; i < 255; i++) {
        buf[i] = '-';
    }
    buf[255] = '\0';

    // Test valid write
    for (unsigned long i = 0; i < sizeof(valid_write_size); i++) {
        printf("Writing to terminal of size %u ...\n", valid_write_size[i]);
        if (valid_write_size[i] == (ret = write(FD_STDOUT, buf, valid_write_size[i]))) {
            printf("... Done\n");
        } else {
            printf("... Fail with return %d\n", ret);
            result = FAIL;
        }
    }

    // Test invalid write
    for (unsigned long i = 0; i < sizeof(invalid_write_size); i++) {
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
    const size_t buf_size = 23;
    char buf[buf_size];
    unsigned long ret;

    // TODO: add some test files
    const char *test_file[] = {""};

    // Read root directory
    if (-1 == (fd = open((uint8_t *) "."))) {
        printf("Failed to open \".\"\n");
        return FAIL;
    }
    while (0 != (ret = read(fd, buf, buf_size - 1))) {
        if (-1 == ret) {
            printf("Failed to read \".\"\n");
            return FAIL;
        }
        buf[ret] = '\n';
        if (-1 == write(FD_STDOUT, buf, ret + 1)) {
            printf("Failed to write to stdout\n");
            return FAIL;
        }
    }
    // TODO: it's not correct to directly read the directory file

    close(fd);

    // Test reading files
    for (unsigned long i = 0; i < sizeof(test_file); i++) {

        // TODO: replace with API of terminal driver
        clear();
        reset_cursor();

        printf("Read %s (%u/%u)\n", test_file[i], i, sizeof(test_file));

        if (-1 == (fd = open((const uint8_t *) test_file[i]))) {
            printf("Failed to open %s\n", test_file[i]);
            return FAIL;
        }

        while (0 != (ret = read(fd, buf, buf_size - 1))) {
            if (-1 == ret) {
                printf("Failed to read %s\n", test_file[i]);
                close(fd);
                return FAIL;
            }
            buf[ret] = '\n';
            if (-1 == write(FD_STDOUT, buf, ret + 1)) {
                printf("Failed to write to stdout\n");
                close(fd);
                return FAIL;
            }
        }

        close(fd);

        printf("Press enter to continue... %s\n", test_file[i]);
        (void) read(FD_STDIN, buf, buf_size - 1);
    }

    return PASS;

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
    TEST_OUTPUT("terminal_test", terminal_test());
    TEST_OUTPUT("fs_test", fs_test());

    printf("\nTests complete. Press F2 to divide 0, F3 to dereference NULL.\n");
}
