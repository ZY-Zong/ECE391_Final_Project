/* idt_handler.h - Low-level handlers for exceptions, interrupts and system calls
*/

#ifndef _IDT_HANDLER_H
#define _IDT_HANDLER_H

#define SYSTEM_CALL_TABLE_SIZE   11

#ifndef ASM

#define IDT_ENTRY_INTEL          0x20  // number of vectors used by intel
#define IDT_ENTRY_KEYBOARD       0x21  // the vector number of keyboard
#define IDT_ENTRY_RTC            0x28  // the vector number of RTC
#define IDT_ENTRY_SYSTEM_CALL    0x80  // the vector number of system calls

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
extern void interrupt_entry_1();
extern void interrupt_entry_8();

// Defined in idt_asm.S
extern void system_call_entry();

extern void idt_init();

#endif // ASM

#endif // _IDT_HANDLER_H
