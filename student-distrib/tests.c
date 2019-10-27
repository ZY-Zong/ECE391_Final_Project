#include "tests.h"
#include "x86_desc.h"
#include "lib.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL")

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

/* Checkpoint 1 tests */

/* Counters for test */
static unsigned long test1_rtc_counter = 0;

/**
 * Helper function for RTC test
 */
void test1_handle_rtc() {
    test1_rtc_counter++;
    if (test1_rtc_counter > 100000) test1_rtc_counter = 0;  // avoid overflow
}

/**
 * Asserts that first 10 IDT entries are not NULL
 * @return PASS/FAIL
 */
int idt_test() {
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
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
int paging_structure_test() {
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
    if ((kernel_page_table_t*) (kernel_page_directory.entry[0] & 0xFFFFF000) != &kernel_page_table_0) {
        printf(PRINT_ERR"Paging directory entry 0 is not correct");
        result = FAIL;
    }
    if ((kernel_page_directory.entry[1] & 0x00400000) != 0x00400000) {
        printf(PRINT_ERR"Paging directory entry 1 is not correct");
        result = FAIL;
    }

    // Check kernel page table 0
    for (i = 0; i < VIDEO_MEMORY_START_PAGE; ++i){
        if (kernel_page_table_0.entry[i] != 0){
            printf(PRINT_ERR"Paging table entry %d is not correct", i);
            result = FAIL;
        }
    }

    // Check video memory configuration
    if ((kernel_page_table_0.entry[VIDEO_MEMORY_START_PAGE] & 0x000B8003) != 0x000B8003){
        printf("Paging table entry for video memory is not correct");
        result = FAIL;
    }
    for (i = VIDEO_MEMORY_START_PAGE + 1; i < KERNEL_PAGE_TABLE_SIZE; ++i){
        if (kernel_page_table_0.entry[i] != 0){
            printf(PRINT_ERR"Paging table entry %d is not correct", i);
            result = FAIL;
        }
    }

    // Try to dereference some variables
    int* i_ptr = &i;
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
int divide_zero_test() {
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
int dereference_null_test() {
    TEST_HEADER;

    // Set i to 0, but avoid compile warnings
    unsigned long i = 42;
    i -= i;

    // Dereference NULL
    unsigned long j = *((unsigned long *)i);

    // To avoid compiler warnings. Should not get here
    printf("j = %u", j);

    return FAIL;
}

/**
 * Test RTC. Return PASS after receiving 1024 RTC interrupts.
 * @return PASS or FAIL
 */
int rtc_test() {
    TEST_HEADER;

    unsigned long tmp;
    printf("Waiting for 1024 RTC interrupts...\n");
    cli(); {
        test1_rtc_counter = 0;
    }
    sti();
    while(1) {
        cli(); {
            tmp = test1_rtc_counter;
        }
        sti();
        if (tmp > 1024) {
            break;
        }
    }
    return PASS;
}

/**
 * Test for MP1
 */
void test1() {

    // Clear screen
    clear();
    reset_cursor();

    TEST_OUTPUT("idt_test", idt_test());
    TEST_OUTPUT("rtc_test", rtc_test());
    TEST_OUTPUT("paging_structure_test", paging_structure_test());

    printf("\nAll auto tests completed. Press F2 to divide 0 or F3 to dereference NULL.\n");

}

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
    test1();
	// launch your tests here
}
