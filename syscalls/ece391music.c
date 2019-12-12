#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

/**
 * Beep
 * @param   frequency: the frequency of beep 
 * @param   time_in_half_sec: the duration of the beep 
 * @effect: the frequency of PIT channel 2 will be changed!
 */
void beep(uint32_t frequency, uint32_t time_in_half_sec){
    
    if (frequency == 0 || time_in_half_sec == 0) return;
    
    // Open a rtc and set frequency
    int i; // loop counter
    uint8_t buf[]="rtc";
    int rtc_fd=ece391_open(buf); // default frequency 2Hz
    
    if (rtc_fd == -1) return;
    
    ece391_playsound(frequency); // turn on the sound 

    // beeeeeeeeee
    for (i = 0; i< time_in_half_sec; i++) ece391_read(rtc_fd, 0, 0);
    
    ece391_nosound(); // turn off the sound 

    ece391_close(rtc_fd);
}

/**
 * Try to transform a ASCII string into int 
 * @param   buf: the buffer for strings 
 * @param   length: the length of the string (not include nul)
 * @return  the number for success, -1 for fail 
 */
int32_t atoi(uint8_t* buf, int32_t length){
    
    if (buf == 0 || length == 0) return -1; 
    
    int i; // loop counter 
    int32_t ret = 0;
    int32_t power_of_10 = 1;

    for (i = 0; i < length; i++){
        if (buf[length - i - 1] < '0' || buf[length - i - 1] > '9') return -1;

        ret += (buf[length - i - 1] - '0') * power_of_10;
        power_of_10 *= 10;
    }

    return ret;

}


int main(){
    uint8_t buf[1024]; // big enough buffer 
    int i; // loop counter 

    if (0 != ece391_getargs (buf, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return -1;
    }

    // Get rid of the spaces 
    for (i=0; i < 1024; i++){
        if (buf[i]!=' ') break;
    }

    // Get the first argument 
    uint8_t first_arg[128];
    int32_t first_arg_length = 0;
    for (; i < 128; i++){
        if ( buf[i] != ' ' && buf[i] != '\0' ){
            first_arg[first_arg_length++]=buf[i];
        } else {
            break;
        }
    }
    uint32_t frequency = atoi(first_arg, first_arg_length);
    if (frequency == -1 || first_arg_length == 0){
        ece391_fdputs (1, (uint8_t*)"input format: <frequency> <time_in_half_sec>\n");
        return -1;
    }

    // Get rid of the spaces 
    for (; i < 1024; i++){
        if (buf[i]!=' ') break;
    }

    // Get the second argument 
    uint8_t second_arg[128];
    int32_t second_arg_length=0;
    for (; i < 128; i++){
        if ( buf[i] != ' ' && buf[i] != '\0' ){
            second_arg[second_arg_length++]=buf[i];
        } else {
            break;
        }
    }
    uint32_t time_in_half_sec = atoi(second_arg, second_arg_length);
    if (time_in_half_sec == -1 || second_arg_length == 0){
        ece391_fdputs (1, (uint8_t*)"input format: <frequency> <time_in_half_sec>\n");
        return -1;
    }

    beep(frequency, time_in_half_sec);

    return 0;
}
