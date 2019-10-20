//
// Created by qig2 on 10/20/2019.
//

#ifndef LOAD_IDT_H
#define LOAD_IDT_H
#define INTEL_DEFINED 0X20 // Number of vectors used by intel
#define KEYBOARD_INDEX 0x21 // The vector number of keyboard
#define RTC_INDEX 0x28 // The vector number of RTC
#define SYSTEM_CALL_INDEX 0x80 // The vector number of system calls
extern void idt_init();
#endif //LOAD_IDT_H
