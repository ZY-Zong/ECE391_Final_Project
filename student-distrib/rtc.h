/* rtc.h - header file for rtc
*/

#ifndef _RTC_H
#define _RTC_H

#include "types.h"

#define RTC_IRQ_NUM   8

typedef struct rtc_control_t rtc_control_t;
struct rtc_control_t {
    int32_t target_freq;
    int32_t counter;
};

void rtc_control_init(rtc_control_t* rtc_control);

/* Initialize the real time clock */
void rtc_init();

/* Read register C after an IRQ 8, or interrupt will not happen again. */
void rtc_restart_interrupt();

/* RTC drivers */
int32_t system_rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t system_rtc_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t system_rtc_open(const uint8_t* filename);
int32_t system_rtc_close(int32_t fd);


// The followings are for system time 
// Tingkai Liu 2019.12.10
// source: https://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC
#define CURRENT_YEAR        2019                            // Change this each year!
 
int century_register = 0x00;                                // Set by ACPI table parsing code if possible
 
unsigned char second;
unsigned char minute;
unsigned char hour;
unsigned char day;
unsigned char month;
// unsigned int year;


// Should be use as a seperate thread! There is infinit loop in it!
void update_system_time();


#endif // _RTC_H

