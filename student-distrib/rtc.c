/* rtc.h - kernel file for rtc
*/

#include "rtc.h"
#include "lib.h"

unsigned int TEST_RTC_ECHO_COUNTER;

/*
 * rtc_init
 *   REFERENCE: https://wiki.osdev.org/RTC
 *   DESCRIPTION: Initialize the real time clock
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Frequency is set to be default 1024 Hz
 */
void rtc_init() {
    // Turn on IRQ 8, default frequency is 1024 Hz

    cli();  // disable interrupts
    {
        outb(RTC_STATUS_REGISTER_B, RTC_REGISTER_PORT);  // select register B and disable NMI
        char prev = inb(RTC_RW_DATA_PORT);    // read the current value of register B
        outb(RTC_STATUS_REGISTER_B,
             RTC_REGISTER_PORT);     // set the index again (a read will reset the index to register D)
        outb (prev | 0x40,
              RTC_RW_DATA_PORT);     // write the previous value ORed with 0x40. This turns on bit 6 of register B
    }
    sti();
}


/*
 * rtc_interrupt_handler
 *   REFERENCE: https://wiki.osdev.org/RTC
 *   DESCRIPTION: Handle the RTC interrupt. Note that Status Register C will contain a bitmask
 *                telling which interrupt happened.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void rtc_interrupt_handler() {

    static unsigned int counter = 0;
    if (++counter >= TEST_RTC_ECHO_COUNTER) {
        printf("------------------------ Receive %u RTC interrupts ------------------------\n",
               TEST_RTC_ECHO_COUNTER);
        counter = 0;
    }
#ifdef RUN_TESTS
    test1_handle_rtc();
#endif

    /* Get another interrupt */
    rtc_restart_interrupt();
}


/*
 * rtc_restart_interrupt
 *   REFERENCE: https://wiki.osdev.org/RTC
 *   DESCRIPTION: Read register C after an IRQ 8, or interrupt will not happen again.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void rtc_restart_interrupt() {
    outb(RTC_STATUS_REGISTER_C, RTC_REGISTER_PORT);    // select register C
    inb(RTC_RW_DATA_PORT);  // just throw away contents
}

/*
 * rtc_open
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t rtc_open() {

}

/*
 * rtc_read
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t rtc_read() {

}

/*
 * rtc_write
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t rtc_write() {

}

/*
 * rtc_close
 *   DESCRIPTION:
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t rtc_close() {

}







