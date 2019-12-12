
#include "beep.h"
#include "lib.h"
#include "rtc.h"
#include "file_system.h"

// reference: https://wiki.osdev.org/PC_Speaker
/* down from here */
//Play sound using built in speaker
int32_t system_play_sound(uint32_t nFrequence) {
 	if (nFrequence == 0 ) return -1;
     
    uint32_t Div;
 	uint8_t tmp;
    uint32_t flag;
 
    cli_and_save(flag); {
        //Set the PIT to the desired frequency
 	    Div = 1193180 / nFrequence;
 	    outb(0xb6, 0x43);
 	    outb((uint8_t) (Div), 0x42);
 	    outb((uint8_t) (Div >> 8), 0x42);
 
        //And play the sound using the PC speaker
 	    tmp = inb(0x61);
  	    if (tmp != (tmp | 3)) {
 	    	outb(tmp | 3, 0x61);
 	    }
    }
    restore_flags(flag);

    return 0;
 }
 
 //make it shutup
int32_t system_nosound() {
 	uint32_t flag;
    
    cli_and_save(flag); {
        uint8_t tmp = inb(0x61) & 0xFC;
 	    outb(tmp, 0x61);
    }
    restore_flags(flag);

    return 0;
    
 }

// reference: https://wiki.osdev.org/PC_Speaker
/* up from here */

