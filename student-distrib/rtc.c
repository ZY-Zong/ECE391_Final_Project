/* rtc.h - kernel file for rtc
*/

#include "rtc.h"

#include "lib.h"
#include "task_sched.h"
#include "i8259.h"

#include "task.h"
#include "idt.h"
#include "task_sched.h"

#define RTC_HARDWARE_FREQUENCY   1024
#define RTC_MAX_FREQUENCY        32768
#define RTC_MIN_RATE    6   // rate for 1024 Hz
#define RTC_MAX_RATE    15  // rate for 2 Hz

/* RTC Status Registers */
#define RTC_STATUS_REGISTER_A   0x8A
#define RTC_STATUS_REGISTER_B   0x8B
#define RTC_STATUS_REGISTER_C   0x8C

/* 2 IO ports used for the RTC and CMOS */
#define RTC_REGISTER_PORT       0x70
#define RTC_RW_DATA_PORT        0x71

// Wait list for RTC
task_list_node_t rtc_wait_list = TASK_LIST_SENTINEL(rtc_wait_list);

/**
 * Initialize RTC control block
 * @param rtc_control
 */
void rtc_control_init(rtc_control_t *rtc_control) {
    rtc_control->target_freq = -1;  // not initialized
}

// Helper function to check whether the input is power of two
int is_power_of_two(int32_t input);

/**
 * Initialize the real time clock
 * @reference https://wiki.osdev.org/RTC
 * @effect    Frequency is set to be default 2 Hz
 */
void rtc_init() {

    uint32_t flags;
    uint8_t prev;

    cli_and_save(flags);
    {
        // Turn on IRQ 8
        outb(RTC_STATUS_REGISTER_B, RTC_REGISTER_PORT);  // select register B and disable NMI
        prev = inb(RTC_RW_DATA_PORT);  // read the current value of register B
        outb(RTC_STATUS_REGISTER_B,
             RTC_REGISTER_PORT);  // set the index again (a read will reset the index to register D)
        outb (prev | 0x40,
              RTC_RW_DATA_PORT);  // write the previous value ORed with 0x40. This turns on bit 6 of register B

        // Set frequency to 1024 Hz
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT);  // set index to register A, disable NMI
        prev = inb(RTC_RW_DATA_PORT);  // get initial value of register A
        outb(RTC_STATUS_REGISTER_A, RTC_REGISTER_PORT);  // reset index to A
        outb((prev & 0xF0) | RTC_MIN_RATE, RTC_RW_DATA_PORT);   // write rate 1024 Hz to A
    }
    restore_flags(flags);
}

/**
 * Handle the RTC interrupt. Note that Status Register C will contain a bitmask telling which interrupt happened.
 * @reference https://wiki.osdev.org/RTC
 */
asmlinkage void rtc_interrupt_handler(hw_context_t hw_context) {
    uint32_t flags;
    int32_t wake_count = 0;

    task_list_node_t* node;
    task_list_node_t* temp;
    task_t* task;

    cli_and_save(flags);
    {
        task_list_for_each_safe(node, &rtc_wait_list, temp) {
            task = task_from_node(node);
            task->rtc.counter--;
            if (task->rtc.counter == 0) {
                task->parent->flags &= ~TASK_WAITING_RTC;
                sched_refill_time(task);
                // Already in lock
                sched_insert_to_head_unsafe(task);
                wake_count++;
            }
        }

        rtc_restart_interrupt();  // to get another interrupt
        idt_send_eoi(hw_context.irq_exp_num);

        if (wake_count > 0) {
            sched_launch_to_current_head();  // insert multiple task to scheduler list head, but launch only once.
        }

    }
    restore_flags(flags);
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
 * Open RTC and set frequency to 2Hz
 * @param filename     Ignore
 * @return 0 on success
 */
int32_t system_rtc_open(const uint8_t *filename) {
    (void) filename;
    running_task()->rtc.target_freq = 2;
    return 0;
}

/**
 * Read data from RTC, wait for interrupt occurring
 * @param fd        Ignore
 * @param buf       Ignore
 * @param nbytes    Ignore
 * @return 0
 */
int32_t system_rtc_read(int32_t fd, void *buf, int32_t nbytes) {

    (void) fd;
    (void) buf;
    (void) nbytes;

    uint32_t flags;

    if (running_task()->rtc.target_freq == -1) {
        DEBUG_ERR("system_rtc_read(): used before system_rtc_open()");
        return -1;
    }

    cli_and_save(flags);
    {
        // Put running task to sleep and refill wait counter
        running_task()->flags |= TASK_WAITING_RTC;

        // Refill the counter
        running_task()->rtc.counter = RTC_HARDWARE_FREQUENCY / running_task()->rtc.target_freq;

        // Move running task out to wait list
        // Already in lock
        sched_move_running_after_node_unsafe(&rtc_wait_list);

        // Yield processor to other task
        sched_launch_to_current_head();
    }
    restore_flags(flags);

    return 0;
}

/**
 * Set RTC frequency
 * @param fd         Ignore
 * @param buf        Pointer to an integer indicating the RTC rate
 * @param nbytes     Must be 4
 * @return 0 on success, -1 on failure
 */
int32_t system_rtc_write(int32_t fd, const void *buf, int32_t nbytes) {

    (void) fd;
    if (buf == NULL) return -1;  // buf pointer can not be NULL
    if (nbytes != 4) return -1;  // nbytes must be 4

    int32_t frequency = *((int32_t *) buf);

    int power = is_power_of_two(frequency);
    if (power == -1) return -1;  // fail if frequency is not power of two

    uint32_t flags;

    cli_and_save(flags);
    {
        // Restore required frequency and count for this task
        running_task()->rtc.target_freq = frequency;
        running_task()->rtc.counter = RTC_HARDWARE_FREQUENCY / frequency;
    }
    restore_flags(flags);

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
 * @param fd    Ignore
 * @return 0 on success
 */
int32_t system_rtc_close(int32_t fd) {
    (void) fd;
    return 0;
}


// The followings are for system time 
// Tingkai Liu 2019.12.10
// source: https://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC
/* down from here */

enum {
      cmos_address = 0x70,
      cmos_data    = 0x71
};
 
int get_update_in_progress_flag() {
      outb(0x0A, cmos_address);
      return (inb(cmos_data) & 0x80);
}
 
unsigned char get_RTC_register(int reg) {
      outb(reg, cmos_address);
      return inb(cmos_data);
}
 
void update_system_time() {

      unsigned char registerB; 
 
      while (get_update_in_progress_flag());                // Make sure an update isn't in progress
      second = get_RTC_register(0x00);
      minute = get_RTC_register(0x02);
      hour = get_RTC_register(0x04);
      day = get_RTC_register(0x07);
      month = get_RTC_register(0x08);
    
 
      registerB = get_RTC_register(0x0B);
 
      // Convert BCD to binary values if necessary
 
      if (!(registerB & 0x04)) {
            second = (second & 0x0F) + ((second / 16) * 10);
            minute = (minute & 0x0F) + ((minute / 16) * 10);
            hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
            day = (day & 0x0F) + ((day / 16) * 10);
            month = (month & 0x0F) + ((month / 16) * 10);
            
      }
 
      // Convert 12 hour clock to 24 hour clock if necessary
 
      if (!(registerB & 0x02) && (hour & 0x80)) {
            hour = ((hour & 0x7F) + 12) % 24;
      }
}

// source: https://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC
/* up from here */



