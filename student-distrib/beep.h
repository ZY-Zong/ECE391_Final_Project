
#ifndef _BEEP_H
#define _BEEP_H

#include "types.h"

// Support for beeper (PIT channel 2)
// To use, should start qemu with command -soundhw pcspk
// Reference: https://wiki.osdev.org/PC_Speaker

void play_sound(uint32_t nFrequence);
void nosound();

/*
 * Version 6.0 Tingkai Liu 2019.12.11
 * First written 
 */

void beep(uint32_t frequency, uint32_t time_in_ms);

#endif /* _BEEP_H */