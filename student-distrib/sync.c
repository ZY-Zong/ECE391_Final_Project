/* sync.c - Functions for synchronization
*/

#include "lib.h"

static int cli_counter = 0;  // times of clearing IF

/**
 * CLI with counter
 */
void inline cli_safe() {
    cli();
    cli_counter++;
}

/**
 * STI with counter
 */
void inline sti_safe() {
    cli_counter--;
    if (cli_counter == 0) {
        sti();
    } else if (cli_counter < 0) {
        DEBUG_ERR("cli_counter INCONSISTENT!");
        cli_counter = 0;
    }
}
