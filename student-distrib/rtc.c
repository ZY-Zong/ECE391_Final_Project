/* rtc.h - kernel file for rtc
*/

#include "rtc.h"
#include "lib.h"
#include "i8259.h"

unsigned int TEST_RTC_ECHO_COUNTER;

static volatile int rtc_interrupt_occured;

// Helper function to check whether the input is power of two
int is_power_of_two(int32_t input);

/**
 * Initialize the real time clock
 * @reference https://wiki.osdev.org/RTC
 * @effect    Frequency is set to be default 2 Hz
 */
void rtc_init() {
    rtc_interrupt_occured = 0;  // reset flag to be 0, no interrupt happens

    // Turn on IRQ 8
//    cli();  // disable interrupts
    {
        outb(RTC_STATUS_REGISTER_B, RTC_REGISTER_PORT);  // select register B and disable NMI
        char prev = inb(RTC_RW_DATA_PORT);  // read the current value of register B
        outb(RTC_STATUS_REGISTER_B,
             RTC_REGISTER_PORT);  // set the index again (a read will reset the index to register D)
        outb (prev | 0x40,
              RTC_RW_DATA_PORT);  // write the previous value ORed with 0x40. This turns on bit 6 of register B
    }
//    sti();

    // Default frequency is 2 Hz
//    cli();  // disable interrupts
    {
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT);  // set index to register A, disable NMI
        char prev = inb(RTC_RW_DATA_PORT);  // get initial value of register A
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT);  // reset index to A
        outb((prev & 0xF0) | RTC_MAX_RATE, RTC_RW_DATA_PORT);   // write rate 2 Hz to A
    }
//    sti();
}

/**
 * Handle the RTC interrupt. Note that Status Register C will contain a bitmask
 *                telling which interrupt happened.
 * @reference https://wiki.osdev.org/RTC
 */
void rtc_interrupt_handler() {

    rtc_interrupt_occured = 1;  // interrupt happens, set flag to 1

    rtc_restart_interrupt();  // get another interrupt
}

/**
 * Read register C after an IRQ 8, or interrupt will not happen again
 * @reference https://wiki.osdev.org/RTC
 */
void rtc_restart_interrupt() {
    outb(RTC_STATUS_REGISTER_C, RTC_REGISTER_PORT);  // select register C
    inb(RTC_RW_DATA_PORT);  // just throw away contents
}

/**
 * Open RTC
 * @param filename Ignore
 * @return         0 on success
 * @effect         Initialize RTC
 */
int32_t rtc_open(const uint8_t* filename) {
    rtc_init();  // initialize RTC, set default frequency to 2 Hz

    return 0;
}

/**
 * Read data from RTC, wait for interrupt occurring
 * @param fd     Ignore
 * @param buf    Ignore
 * @param nbytes Ignore
 * @return       0 on success
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {

    int flag_t;

    rtc_interrupt_occured = 0;  // set flag to 0

    rtc_restart_interrupt();  // take another interrupt, interrupt available 0.5s after rtc_read

    do {
        cli();  // disable interrupts
        {
            flag_t = rtc_interrupt_occured;
        }
        sti();
    } while (!flag_t); // wait for the interrupt occurs

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

    if (buf == NULL) return -1;  // buf pointer can not be NULL
    if (nbytes != 4) return -1;  // nbytes must be 4

    int32_t frequency =  *((int32_t *) buf);
    int power = is_power_of_two(frequency);

    if (power == -1) {
        return -1;
    }  // fail if frequency is not power of two

    int rate_t = (RTC_MAX_RATE - power) + 1;  // frequency = 32768 >> (rate - 1)
    if (rate_t < RTC_MIN_RATE) return -1;  // RTC allows interrupt frequency up to 1024 Hz

    rate_t &= 0x0F;  // rate must be in the range [2,15]

    // Set the rate of periodic interrupt
    cli();  // disable interrupts
    {
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT);  // set index to register A, disable NMI
        char prev = inb(RTC_RW_DATA_PORT);  // get initial value of register A
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT);  // reset index to A
        outb((prev & 0xF0) | rate_t, RTC_RW_DATA_PORT);  // write rate to A
    }
    sti();

    return 0;
}

/**
 * Helper function to check whether the input value is power of two or not
 * @param input Value to be checked
 * @return      -1 if input is not power of 2, the power value otherwise
 */
int is_power_of_two(int32_t input) {
    int power = 0;
    unsigned int remainder = (unsigned int) input;

    while (remainder > 1) {
        if ((remainder & 0x01) == 1) {
            return -1;
        }  // not power of two

        remainder = remainder >> 1;
        power++;
    }
    return power;
}

/**
 * Close RTC file descriptor
 * @param fd Ignore
 * @return   0 on success
 */
int32_t rtc_close(int32_t fd) {
    return 0;
}



