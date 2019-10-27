/* rtc.h - header file for rtc
*/

#ifndef RTC_H
#define RTC_H

#include "types.h"

/** RTC related constants */
#define RTC_IRQ_NUM   8
#define RTC_DEFAULT_FREQUENCY   1024
#define RTC_MAX_FREQUENCY   32768
#define RTC_MIN_RATE    6   // rate for 1024 Hz
#define RTC_MAX_RATE    15  // rate for 2 Hz

/* RTC Status Registers */
#define RTC_STATUS_REGISTER_A   0x8A
#define RTC_STATUS_REGISTER_B   0x8B
#define RTC_STATUS_REGISTER_C   0x8C

/* 2 IO ports used for the RTC and CMOS */
#define RTC_REGISTER_PORT       0x70
#define RTC_RW_DATA_PORT        0x71

/* Initialize the real time clock */
void rtc_init();

/* Handle the RTC interrupt */
void rtc_interrupt_handler();

/* Read register C after an IRQ 8, or interrupt will not happen again. */
void rtc_restart_interrupt();

/* RTC drivers */
int32_t rtc_open(const char *filename, int flags, int mode);
int32_t rtc_read(unsigned int fd, char *buf, size_t count);
int32_t rtc_write(unsigned int fd, const char *buf, size_t count);
int32_t rtc_close(unsigned int fd);

#endif // RTC_H

