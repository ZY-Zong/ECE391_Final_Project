/* rtc.h - header file for rtc
*/

#ifndef _RTC_H
#define _RTC_H

#include "types.h"

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

typedef struct rtc_control_t rtc_control_t;
struct rtc_control_t {
    int32_t target_freq;
    int32_t counter;
};

// TODO: implement this function
void rtc_control_init(rtc_control_t* rtc_control);

/* Initialize the real time clock */
void rtc_init();

/* Handle the RTC interrupt */
void rtc_interrupt_handler();

/* Read register C after an IRQ 8, or interrupt will not happen again. */
void rtc_restart_interrupt();

/* RTC drivers */
// TODO: revise these function to support virtual RTC
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);

#endif // _RTC_H

