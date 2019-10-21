#include "tests.h"
#include "x86_desc.h"
#include "lib.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

static unsigned int test1_password_state = 0;
static unsigned int test1_rtc_counter = 0;

/**
 *
 * @param c
 */
void test1_handle_typing(char c) {
    if (test1_password_state == 0 && c == 'e') test1_password_state = 1;
    else if (test1_password_state == 1 && c == 'c') test1_password_state = 2;
    else if (test1_password_state == 1 && c == 'e') test1_password_state = 3;
    else if (test1_password_state == 1 && c == '3') test1_password_state = 4;
}

void test1_handle_rtc() {

}

void test1() {
    printf("\nEnter ece391 to start test\n");

}

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
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

/* Divide Zero Test
 *
 * Try to divide 0
 * Inputs: None
 * Outputs: None
 * Side Effects: Cause Divide Error
 * Coverage: Divide Error exception
 * Files: boot.S, kernel.C
 */
void divide_zero_test() {
    TEST_HEADER;

    // Set i to 0, but avoid compile warnings
    unsigned long i = 42;
    i -= i;

    // Divide 0
    unsigned long j = 42 / i;

    // To avoid compiler warnings. Should not get here
    printf("j = %u", j);
}

/* Deference NULL Test
 *
 * Try to dereference NULL
 * Inputs: None
 * Outputs: None
 * Side Effects: Cause Page Fault if paging is turning on
 * Coverage: Page Fault exception
 * Files: boot.S, kernel.C
 */
void dereference_null_test() {
    TEST_HEADER;

    // Set i to 0, but avoid compile warnings
    unsigned long i = 42;
    i -= i;

    // Dereference NULL
    unsigned long j = *((unsigned long *)i);

    // To avoid compiler warnings. Should not get here
    printf("j = %u", j);
}

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
}
