//
// Created by qig2 on 10/20/2019.
//
/* This file defines the function needed to initialize IDT.
 *
 * Caution:
 * This version only initialize the idt table, leaving out the first 32 exceptions defined by Intel.
 * Also, the interrupt handler pointer for keyboard and RTC is not set.
 * System call pointer also is not set.
 */
#include "load_idt.h"
#include "x86_desc.h"
/*
 * idt_init
 * This function is used to initialize IDT table and called in kernel.c.
 * Uses subroutine provided in x86_desc.h.
 * Input: None.
 * Output: None.
 * Side effect: Change the IDT table defined in x86_desc.S.
 */
extern void idt_init() {
    int i;
    // Initialize 0x00-0x1F exception handler defined by Intel.
    for (i = 0; i < INTEL_DEFINED; i++) {
        idt[i].seg_selector = KERNEL_CS;
        idt[i].dpl = 0;
    }
    // Initialize 0x20-0xFF general purpose interrupt handlers
    for (i = INTEL_DEFINED; i < NUM_VEC; i++) {
        idt[i].seg_selector = KERNEL_CS;
        idt[i].dpl = 0;
    }
    // Initialize keyboard
    {
        // After writing the keyboard handler, uncomment and fill in the pointer
        // SET_IDT_ENTRY(idt[KEYBOARD_INDEX], );
    }
    // Initialize RTC
    {
        // After writing the RTC handler, uncomment and fill in the pointer
        // SET_IDT_ENTRY(idt[RTC_INDEX], );
    }
    // Initialize system calls
    {
        // After writing the RTC handler, uncomment and fill in the pointer
        // SET_IDT_ENTRY(idt[SYSTEM_CALL_INDEX], );
        idt[SYSTEM_CALL_INDEX].dpl = 3;
    }
}

