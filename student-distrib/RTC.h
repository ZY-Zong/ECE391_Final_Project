/*									tab:8
 *
 * RTC.h - header file for Real Time Clock device
 * Author:	    Qi Gao
 *              Tingkai Liu
 *              Zikai Liu
 *              Zhenyu Zong
 * Creation Date:   Sat Oct 19 23:52 2019
 * Filename:        RTC.h
 */

#ifndef RTC_H
#define RTC_H

#define RTC_IRQ     8   /* RTC IRQ number */

/* RTC Status Registers */
#define RTC_STATUS_REGISTER_A   0x0A
#define RTC_STATUS_REGISTER_B   0x0B
#define RTC_STATUS_REGISTER_C   0x0C

/* 2 IO ports used for the RTC and CMOS */
#define RTC_REGISTER_PORT       0x70
#define RTC_RW_DATA_PORT        0x71

#define RTC_DEFAULT_FREQUENCY   1024

/* Initialize RTC */
void rtc_init();

/* Handle RTC interrupt */
void rtc_interrupt_handler();

#endif /* RTC_H */