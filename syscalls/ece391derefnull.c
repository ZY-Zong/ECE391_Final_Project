#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    ece391_fdputs(1, (uint8_t*) "This program is going to dereference NULL!");

    // Set i to 0, but avoid compile warnings
    unsigned long i = 42;
    i -= i;

    // Dereference NULL
    unsigned long j = *((unsigned long *) i);

    (void) j;
    ece391_fdputs(1, (uint8_t*) "[Error] dereference NULL does not cause an exception!");

    return 0;
}

