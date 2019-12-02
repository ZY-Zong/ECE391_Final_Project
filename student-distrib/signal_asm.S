
#define ASM_SIGNAL  1

#include "signal.h"

.global     signal_set_up_stack_helper
.global     system_sigreturn
#.extern     signal_handle   # the adress of signal handler funtion, defined in signal.c
#.extern     signal_restore_mask # helper function for restore signal mask, defined in signal.c

# Helper function for setting uo stack for signal handler 
# INPUTS:  
#       the address of hw_context struct 
#       the signal number
# OUTPUTS:
#       none
# Registers:
#       C calling convention
signal_set_up_stack_helper:
    # Set up stack frame 
    pushl   %ebp 
    movl    %esp, %ebp 
    # Save callee save registers 
    pushl   %ebx 
    pushl   %esi
    pushl   %edi   

    # Get the user stack  
    movl    8(%ebp), %eax   # signal handler address
    movl    %eax, temperory_save_2
    movl    12(%ebp), %ecx   # signal number
    movl    16(%ebp), %ebx  # address of hw_context 
    movl    60(%ebx), %esi  # user esp 
    movl    20(%ebx), %edi  # user ebp 

    # Swtich to user stack 
    movl    %esp, temperory_save_0
    movl    %ebp, temperory_save_1
    movl    %edi, %ebp 
    movl    %esi, %esp 

    # Push stuff 
    # Assemble linkage that call sigreturn, in total 7 bytes
    # We will first move first 4 bytes, then move last 4 bytes and over lap with last byte of first move
    subl    $7, %esp
    movl    code_0, %eax
    movl    %eax, (%esp)
    movl    code_0+3, %eax
    movl    %eax, 3(%esp)

    # Store this address for return address of handler
    movl    %esp, %edx

    subl    $1, %esp    # restore 4-byte alignment

    # hw_context 
    pushl   64(%ebx)    # ss
    pushl   60(%ebx)    # esp
    pushl   56(%ebx)    # eflags 
    pushl   52(%ebx)    # cs 
    pushl   48(%ebx)    # eip 
    pushl   44(%ebx)    # err_code / dummy 
    pushl   40(%ebx)    # irq_exp_num
    pushl   36(%ebx)    # fs
    pushl   32(%ebx)    # es 
    pushl   28(%ebx)    # ds 
    pushl   24(%ebx)    # eax 
    pushl   20(%ebx)    # ebp 
    pushl   16(%ebx)    # edi 
    pushl   12(%ebx)    # esi
    pushl   8(%ebx)     # edx 
    pushl   4(%ebx)     # ecx
    pushl   0(%ebx)     # ebx
     
    # parameters for signal handler: signal number 
    pushl   %ecx 
    # the return address of signal handler: the assemble linkage that call sigreturn 
    pushl   %edx 

    # Update the register value in kernel hw_context 
    movl    %esp, 60(%ebx)  # set user ESP
    # Notice: we don't change CS and SS and assume they are USER. Do not check_signal when return to kernel
    # No need to update user EBP

    # Change eip to signal handler
    movl    temperory_save_2, %eax
    movl    %eax, 48(%ebx)

    # Switch back to kernel stack
    movl    temperory_save_0, %esp 
    movl    temperory_save_1, %ebp 
    
    # Restore callee save registers 
    popl    %edi 
    popl    %esi 
    popl    %ebx 

    leave 
    ret


# assemble_linkage_that_call_sigreturn
# will be pushed onto the user stack
# In total 7 bytes
code_0:    movl    $10, %eax 
           int     $0x80
# never back here

# Low-level handler of system call sigreturn()
# Copy the hardware context stored in user stack into kernel stack
# that store these when entering sustem call
system_sigreturn:

    /* TODO: how to stop user from calling sigreturn by accident? */

    /* This function is called by system_call_entry. The top of the stack is return address, and then hw context */
    /* No need to save registers since after its return hw context before signal handling will be restored */


    /* Get the user ESP (pointing to signal number, one place above hw_context in user stack) */
    /* kernel hw_context is at 4(%esp) */
    movl    64(%esp), %esi   # user ESP
    addl    $4, %esi    # now esi point to the top of user hw_context
    movl    %esp, %edi
    addl    $4, %edi    # now edi point to the top of kernel hw_context

    /* Copy the hardware context */
    xorl    %ecx, %ecx  /* loop counter */
    copy_hw_context_loop:
        movl    (%esi, %ecx, 4), %eax 
        movl    %eax, (%edi, %ecx, 4)
        incl    %ecx 
        cmpl    $17, %ecx   /* hw_context has 17 elements */
        jne     copy_hw_context_loop

    # Restore the mask by calling a C funtion
    pushl   %eax 
    pushl   %ecx 
    pushl   %edx 
    call    signal_restore_mask 
    popl    %edx 
    popl    %ecx   
    popl    %eax 
    
    /* Return value: the return of previous system call/interrupt/exception, which has been wrote to hw context
     * before first copying hw context to user stack
     */
    movl    24(%edi), %eax 

    /* No stack frame */

    ret

# data 
temperory_save_0:
    .long   0 
temperory_save_1:
    .long   0
temperory_save_2:
    .long   0

