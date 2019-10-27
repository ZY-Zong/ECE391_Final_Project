/* rtc.h - header file for rtc
*/

#ifndef RTC_H
#define RTC_H

/** RTC related constants */
#define RTC_IRQ_NUM   8
#define RTC_DEFAULT_FREQUENCY   1024
#define RTC_MAX_FREQUENCY   32768
#define RTC_MIN_RATE    3
#define RTC_MAX_RATE    15

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

#endif // RTC_H

