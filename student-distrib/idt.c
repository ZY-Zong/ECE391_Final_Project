/* idt_asm.S - Kernel functions for exceptions, interrupts and system calls
*/

#include "idt.h"

#include "x86_desc.h"
#include "lib.h"
#include "linkage.h"
#include "i8259.h"
#include "file_system.h"
#include "task.h"
#include "task_paging.h"

/**
 * This function is used to initialize IDT table and called in kernel.c. Uses subroutine provided in x86_desc.h.
 * @effect: Change the IDT table defined in x86_desc.S.
 */
void idt_init() {

    int i;

    // Initialize 0x00 - 0x1F exception handler defined by Intel.
    for (i = 0; i < IDT_ENTRY_INTEL; i++) {
        idt[i].seg_selector = KERNEL_CS;
        idt[i].dpl = 0;
        idt[i].size = 1;
        idt[i].present = 1;
        idt[i].reserved0 = 0;
        idt[i].reserved1 = 1;
        idt[i].reserved2 = 1;
        idt[i].reserved3 = 1;
        idt[i].reserved4 = 0;
    }
    // Initialize 0x20 - 0xFF general purpose interrupt handlers
    for (i = IDT_ENTRY_INTEL; i < NUM_VEC; i++) {
        idt[i].seg_selector = KERNEL_CS;
        idt[i].dpl = 0;
        idt[i].size = 1;
        idt[i].present = 0;
        idt[i].reserved0 = 0;
        idt[i].reserved1 = 1;
        idt[i].reserved2 = 1;
        idt[i].reserved3 = 0;
        idt[i].reserved4 = 0;

    }

    // Setup exception handlers (defined in idt_asm.S)
    SET_IDT_ENTRY(idt[0], exception_entry_0);
    SET_IDT_ENTRY(idt[1], exception_entry_1);
    SET_IDT_ENTRY(idt[2], exception_entry_2);
    SET_IDT_ENTRY(idt[3], exception_entry_3);
    SET_IDT_ENTRY(idt[4], exception_entry_4);
    SET_IDT_ENTRY(idt[5], exception_entry_5);
    SET_IDT_ENTRY(idt[6], exception_entry_6);
    SET_IDT_ENTRY(idt[7], exception_entry_7);
    SET_IDT_ENTRY(idt[8], exception_entry_8);
    SET_IDT_ENTRY(idt[9], exception_entry_9);
    SET_IDT_ENTRY(idt[10], exception_entry_10);
    SET_IDT_ENTRY(idt[11], exception_entry_11);
    SET_IDT_ENTRY(idt[12], exception_entry_12);
    SET_IDT_ENTRY(idt[13], exception_entry_13);
    SET_IDT_ENTRY(idt[14], exception_entry_14);
    SET_IDT_ENTRY(idt[16], exception_entry_16);
    SET_IDT_ENTRY(idt[17], exception_entry_17);
    SET_IDT_ENTRY(idt[18], exception_entry_18);
    SET_IDT_ENTRY(idt[19], exception_entry_19);
    SET_IDT_ENTRY(idt[20], exception_entry_20);
    SET_IDT_ENTRY(idt[30], exception_entry_30);

    // Set PIT handler (defined in idt_asm.S)
    SET_IDT_ENTRY(idt[IDT_ENTRY_PIT], interrupt_entry_0);
    idt[IDT_ENTRY_PIT].present = 1;

    // Set keyboard handler (defined in idt_asm.S)
    SET_IDT_ENTRY(idt[IDT_ENTRY_KEYBOARD], interrupt_entry_1);
    idt[IDT_ENTRY_KEYBOARD].present = 1;

    // Set RTC handler (defined in boot.S)
    SET_IDT_ENTRY(idt[IDT_ENTRY_RTC], interrupt_entry_8);
    idt[IDT_ENTRY_RTC].present = 1;

    // Set system calls handler (defined in idt_asm.S)
    SET_IDT_ENTRY(idt[IDT_ENTRY_SYSTEM_CALL], system_call_entry);
    idt[IDT_ENTRY_SYSTEM_CALL].present = 1;
    idt[IDT_ENTRY_SYSTEM_CALL].dpl = 3;

    // Load IDT into IDTR
    lidt(idt_desc_ptr);
}

void idt_send_eoi(uint32_t irq_num) {
    send_eoi(irq_num);
}

/**
 * This function is used to print out the given interrupt number in the interrupt descriptor table.
 * @param vec_num    vector number of the interrupt/exception
 */
void print_exception(uint32_t vec_num) {

#if IDT_EXCEPTION_BACKTRACK

#else
    if (task_count == 0) {
        clear();
        reset_cursor();
        printf("EXCEPTION %u OCCUR IN PURE KERNEL STATE!\n", vec_num);
        printf("------------------------ BLUE SCREEN ------------------------");
    } else {
        DEBUG_ERR("EXCEPTION %u OCCUR!", vec_num);
        system_halt(256);
    }


    volatile int inf_loop = 1;  // set it to 0 in gdb to return to exception content
    while (inf_loop) {}   // put kernel into infinite loop
#endif

}

/**
 * Print message that an interrupt handler is not implemented
 * @param irq    IRQ number
 */
asmlinkage void null_interrupt_handler(uint32_t irq) {
    DEBUG_ERR( "Interrupt handler for IRQ %u is not implemented", irq);
}

/**
 * Print message that an system call is not implemented and print related resigers
 * @usage In system call table in idt.S
 * @return -1
 */
asmlinkage long sys_not_implemented() {
    long eax_val, ebx_val, ecx_val, edx_val; // Variables used to store the registers.
    asm volatile ("                \n \
            movl    %%eax, %0      \n \
            movl    %%ebx, %1      \n \
            movl    %%ecx, %2      \n \
            movl    %%edx, %3      \n \
            "
    : "=r"(eax_val), "=r"(ebx_val), "=r"(ecx_val), "=r"(edx_val)
    :
    : "memory", "cc"
    );
    DEBUG_ERR("Invalid system call: \n    EAX: %d  EBX: %d ECX: %d  EDX: %d",
           eax_val, ebx_val, ecx_val, edx_val);
    return -1;
}

/**
 * Low-level system call handler for execute()
 * @param command    Command to be executed
 * @return Terminate status of the program (0-255 if program terminate by calling halt(), 256 if exception occurs)
 * @note New program given in command will run immediately, and this function will return after its terminate
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_execute(uint8_t *command) {
    return system_execute(command, 1, 0, NULL);
}

/**
 * Low-level system call handler for halt()
 * @param status    Exit code of current process (size are enlarged to support 256 return from exception)
 * @return This function should never return
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_halt(int32_t status) {
    return system_halt(status);
}

/**
 * Low-level system call handler for read()
 * @param fd        File descriptor
 * @param buf       Buffer to store output
 * @param nbytes    Maximal number of bytes to write
 * @return 0 on success, -1 on failure
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_read(int32_t fd, void* buf, int32_t nbytes) {
    return system_read(fd, buf, nbytes);
}

/**
 * Low-level system call handler for write()
 * @param fd        File descriptor
 * @param buf       Buffer of content to write
 * @param nbytes    Number of bytes to write
 * @return 0 on success, -1 on failure
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_write(int32_t fd, const void* buf, int32_t nbytes) {
    return system_write(fd, buf, nbytes);
}

/**
 * Low-level system call handler for open()
 * @param filename    String of filename to open
 * @return 0 on success, -1 on failure
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_open(const uint8_t* filename) {
    return system_open(filename);
}

/**
 * Low-level system call handler for close()
 * @param fd    File descriptor
 * @return 0 on success, -1 on failure
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_close(int32_t fd) {
    return system_close(fd);
}

/**
 * Low-level system call handler for getargs()
 * @param buf       String buffer to accept args
 * @param nbytes    Maximal number of bytes to write to buf
 * @return 0 on success, -1 on no argument or argument string can't fit in nbytes
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_getargs(uint8_t *buf, int32_t nbytes) {
    return system_getargs(buf, nbytes);
}

/**
 * Low-level system call handler for vidmap()
 * @param screen_start    Starting address of mapped video memory in user memory
 * @return 0 on success
 * @usage System call jump table in idt.S
 * @note Arguments of this function is actually saved registers on the stack, so DO NOT modify them in this layer
 */
asmlinkage int32_t lowlevel_sys_vidmap(uint8_t ** screen_start) {
    return system_vidmap(screen_start);
}
