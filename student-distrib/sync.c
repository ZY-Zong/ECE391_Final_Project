/* sync.c - Functions for synchronization
*/

#define lock_save(flags) asm volatile (" \
    pushfl  /* push flags */         \n\
    popl %0 /* pop flags */          \n\
    cli     /* disable interrupts */" \
    : "=rm" (flags) \
    : \
    : "cc", "memory")

#define unlock_restore(flags) asm volatile (" \
    pushl %0  \n\
    popfl" \
    : \
    : "rm" (flags) \
    : "cc", "memory")
