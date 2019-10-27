/* rtc.h - kernel file for rtc
*/

#include <opencl-c.h>
#include "rtc.h"
#include "lib.h"
#include "i8259.h"

unsigned int TEST_RTC_ECHO_COUNTER;
static int32_t rate;

/* Helper function to check whether the input is power of two */
int is_power_of_two(int32_t input);

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
    rtc_interrupt_occured = 0;

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

    /* Set flag to 1 */
    rtc_interrupt_occured = 1;

    rate &= 0x0F; // rate must be in the range [2,15]

    /* Set the rate of periodic interrupt */
    cli(); // disable interrupts
    {
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT); // set index to register A, disable NMI
        char prev = inb(RTC_RW_DATA_PORT); // get initial value of register A
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT); // reset index to A
        outb((prev & 0xF0) | rate, RTC_RW_DATA_PORT);  //write only our rate to A. Note, rate is the bottom 4 bits.
    }
    sti();

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
 *   INPUTS: ignored
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t rtc_open(const uint8_t* filename) {
    /* Set frequency to 2Hz when open */
    rate = RTC_MAX_RATE;    // frequency = 32768 >> (rate - 1) => 2 Hz

    return 0;
}

/*
 * rtc_read
 *   DESCRIPTION: Read data from RTC
 *   INPUTS: ignored
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if success
 *   SIDE EFFECTS: wait for the interrupt if it didn't happen
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {

    /* Set flag to 0 */
    rtc_interrupt_occured = 0;

    /* Wait for the interrupt occur */
    // TODO: may need cli and sti
    while (!rtc_interrupt_occured) {}

    return 0;
}

/**
 * Write data to RTC
 * @param fd      Ignore
 * @param buf     pointer to an integer indicating the RTC rate
 * @param nbytes  Must be 4
 * @return        0 on success, -1 on failure
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {

    if (buf == NULL) return -1;
    if (nbytes != 4) return -1;

    int32_t frequency =  *((int32_t *) buf);
    int power = is_power_of_two(frequency);

    /* If frequency to be set is not power of 2, fail */
    if (power == -1) {
        rate = 0;
        return -1;
    }

    int rate_t = (RTC_MAX_RATE - power) + 1;  // frequency = 32768 >> (rate - 1)
    if (rate_t < RTC_MIN_RATE) { return -1; }  // RTC allows interrupt frequency up to 1024 Hz

    rate = rate_t;
    return 0;
}

/*
 * is_power_of_two
 *   DESCRIPTION: Check whether the input value is power of two or not
 *   INPUTS: input -- 4-byte value to be checked
 *   OUTPUTS: none
 *   RETURN VALUE: -1 if input is not power of 2
 *                 the value of the power otherwise
 *   SIDE EFFECTS: none
 */
int is_power_of_two(int32_t input) {
    int power = 0;
    unsigned int remainder = (unsigned int) input;

    while (remainder > 1) {
        /* Not power of two */
        if ((remainder & 0x01) == 1) {
            return -1;
        }

        remainder = remainder >> 1;
        power++;
    }
    return power;
}

/*
 * rtc_close
 *   DESCRIPTION: Close RTC file descriptor
 *   INPUTS: ignored
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t rtc_close(int32_t fd) {
    return 0;
}



