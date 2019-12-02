
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
    movl    8(%ebp), %ecx   # signal number
    movl    12(%ebp), %ebx  # address of hw_context 
    movl    60(%ebx), %esi  # user esp 
    movl    20(%ebx), %edi  # user ebp 

    # Swtich to user stack 
    movl    %esp, temperory_save_0
    movl    %ebp, temperory_save_1
    movl    %edi, %ebp 
    movl    %esi, %esp 

    # Push stuff 
    # assemble linkage that call sigreturn 
    movl    code_1, %eax 
    pushl   %eax 
    movl    code_0, %eax 
    pushl   %eax 
    # store this address for return address of handler 
    movl    %esp, %edx 
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
    movl    %esp, 60(%ebx)
    movl    %ebp, 20(%ebx)      # actually the same
    /* FIXME: EBP is not correct, should be set to the same as ESP, but the original one should be stored? */
    # change eip to signal handler 
    movl    $signal_handle, %eax
    /* FIXME: should only jump to user program. If the handler is default handler, don't even need to do these */
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
code_0:    movl    $10, %eax 
code_1:    int     $0x80
    # never back here 

# System call: sigreturn 
# Copy the hardware context stored in user stack into kernel stack 
# that store these when entering sustem call 
# INPUTS: none 
# OUTPUTS: always 0
# Registers: C calling convention
system_sigreturn:
    # Set up stack frame 
    pushl   %ebp 
    movl    %esp, %ebp 
    # Save callee save registers 
    pushl   %ebx 
    pushl   %esi
    pushl   %edi   

    # get the user esp (pointing to signal number, one place above user hw_context)
    movl    68(%ebp), %esi 
    # the top of kernel hw_context is at 8(%ebp)

    # point to the top of user and kernel hw_context 
    addl    $4, %esi    # now esi point to the top of user hw_context
    movl    %ebp, %edi 
    addl    $8, %edi    # now edi point to the top of kernel hw_context 

    # copy the hardware context 
    xorl    %ecx, %ecx  # loop counter 
    copy_hw_context_loop:
        movl    (%esi, %ecx, 4), %eax 
        movl    %eax, (%edi, %ecx, 4)
        incl    %ecx 
        cmpl    $17, %ecx   # hw_context has 17 elements 
        jne     copy_hw_context_loop

    # restore the mask by calling a C funtion 
    pushl   %eax 
    pushl   %ecx 
    pushl   %edx 
    call    signal_restore_mask 
    popl    %edx 
    popl    %ecx   
    popl    %eax 
    
    # return value: the return of previous system call/interrupt/exception
    movl    24(%edi), %eax 

    # Restore callee save registers 
    popl    %edi 
    popl    %esi 
    popl    %ebx 

    leave 
    ret



# data 
temperory_save_0:
    .long   0 
temperory_save_1:
    .long   0


