#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    ece391_fdputs(1, (uint8_t*) "This program is going to dividing zero!\n");

    unsigned long i = 42;
    i -= i;

    // Divide 0
    unsigned long j = 42 / i;

    (void) j;
    ece391_fdputs(1, (uint8_t*) "[Error] Dividing 0 does not cause an exception!\n");

    return -1;
}

