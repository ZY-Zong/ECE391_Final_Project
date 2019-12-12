/* idt_handler.h - Low-level handlers for exceptions, interrupts and system calls
*/

#ifndef _IDT_HANDLER_H
#define _IDT_HANDLER_H

#define SYSTEM_CALL_TABLE_SIZE   11

#ifndef ASM

#include "types.h"
#include "linkage.h"

#define EXCEPTION_HANDLING_TYPE    2  // 0 for simply loop, 1 for halting user program, 2 for sending signals

#define IDT_ENTRY_INTEL            0x20  // number of vectors used by intel
#define IDT_ENTRY_PIT              0x20  // the vector number of PIT
#define IDT_ENTRY_KEYBOARD         0x21  // the vector number of keyboard
#define IDT_ENTRY_RTC              0x28  // the vector number of RTC
#define IDT_ENTRY_MOUSE          0x2C  // the vector number of mouse
#define IDT_ENTRY_SYSTEM_CALL      0x80  // the vector number of system calls

typedef struct hw_context_t hw_context_t;
struct hw_context_t {
    int32_t ebx;
    int32_t ecx;
    int32_t edx;
    int32_t esi;
    int32_t edi;
    int32_t ebp;
    int32_t eax;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t irq_exp_num;  // IRQ # or exception #
    uint32_t err_code;  // error code for exception or dummy
    uint32_t eip;  // return address
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
};

#define IDT_INTERRUPT_COUNT      16

// Defined in idt_asm.S
extern void exception_entry_0();
extern void exception_entry_1();
extern void exception_entry_2();
extern void exception_entry_3();
extern void exception_entry_4();
extern void exception_entry_5();
extern void exception_entry_6();
extern void exception_entry_7();
extern void exception_entry_8();
extern void exception_entry_9();
extern void exception_entry_10();
extern void exception_entry_11();
extern void exception_entry_12();
extern void exception_entry_13();
extern void exception_entry_14();
extern void exception_entry_16();
extern void exception_entry_17();
extern void exception_entry_18();
extern void exception_entry_19();
extern void exception_entry_20();
extern void exception_entry_30();

// Defined in idt_asm.S
extern void interrupt_entry_0();
extern void interrupt_entry_1();
extern void interrupt_entry_8();
extern void interrupt_entry_12();

// Defined in idt_asm.S
extern void system_call_entry();

void idt_init();
void idt_send_eoi(uint32_t irq_num);

#endif // ASM

#endif // _IDT_HANDLER_H
