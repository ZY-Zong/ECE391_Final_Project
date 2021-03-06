#ifndef TESTS_H
#define TESTS_H

// Echo interval of RTC for test
#define TEST_RTC_ECHO_COUNTER    5000

void test1_handle_typing(char c);
void test1_handle_rtc();

long divide_zero_test();
long dereference_null_test();


void checkpoint_task_closed_all_files();
void checkpoint_task_paging_consistent();

// test launcher
void launch_tests();

#endif /* TESTS_H */
