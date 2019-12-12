
#ifndef _BEEP_H
#define _BEEP_H

#include "types.h"

// Support for beeper (PIT channel 2)
// To use, should start qemu with command -soundhw all
// Reference: https://wiki.osdev.org/PC_Speaker

int32_t system_play_sound(uint32_t nFrequence);
int32_t system_nosound();

/*
 * Version 6.0 Tingkai Liu 2019.12.11
 * First written 
 */


#endif /* _BEEP_H */