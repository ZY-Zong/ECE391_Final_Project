#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

typedef enum {
        Do1L = 262,     // 261.63Hz 
        Re2L = 294,     // 293.66Hz 
        Mi3L = 330,     // 329.63Hz 
        Fa4L = 349,     // 349.23Hz 
        So5L = 392,     // 392.00Hz 
        La6L = 440,     // 440.00Hz 
        Si7L = 494,     // 493.88Hz 

        Do1M = 523,     // 523.25Hz 
        Re2M = 587,     // 587.33Hz 
        Mi3M = 659,     // 659.26Hz 
        Fa4M = 698,     // 698.46Hz 
        So5M = 784,     // 784.00Hz 
        La6M = 880,     // 880.00Hz 
        Si7M = 988,     // 987.77Hz 

        Do1H = 1047,     // 1046.50Hz
        Re2H = 1175,     // 1174.66Hz
        Mi3H = 1319,     // 1318.51Hz
        Fa4H = 1397,     // 1396.91Hz
        So5H = 1568,     // 1567.98Hz
        La6H = 1760,     // 1760.00Hz
        Si7H = 1976,     // 1975.53Hz

        Silent = 0,
        Finish = -1,
        InfLoop = -2

    } tone_t;

typedef struct song_interval_t {
    int32_t frequency;
    int32_t time_in_half_sec;
} song_interval_t;



/**
 * Beep
 * @param   frequency: the frequency of beep 
 * @param   time_in_half_sec: the duration of the beep 
 * @effect: the frequency of PIT channel 2 will be changed!
 */
void beep(uint32_t frequency, uint32_t time_in_half_sec){
    
    if (time_in_half_sec == 0) return;
    
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

void play_music(music_num){
    switch (music_num){
        case 0: 
            beep(Do1H, 1);
            beep(Re2H, 1);
            beep(Mi3H, 1);
            beep(Fa4H, 1);
            beep(So5H, 1);
            beep(Fa4H, 1);
            beep(Mi3H, 1);
            beep(Re2H, 1);
            beep(Do1H, 1);
            break;
        case 1: 
            beep(Do1H, 1);
            beep(Do1H, 1);
            beep(So5H, 1);
            beep(So5H, 1);
            beep(La6H, 1);
            beep(La6H, 1);
            beep(So5H, 1);
            beep(0,1);
            beep(Fa4H, 1);
            beep(Fa4H, 1);
            beep(Mi3H, 1);
            beep(Mi3H, 1);
            beep(Re2H, 1);
            beep(Re2H, 1);
            beep(Do1H, 1);
            break;
        default: 
            break;
    }
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

int32_t main(){
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
    uint32_t music_num = atoi(first_arg, first_arg_length);
    if (music_num == -1 || first_arg_length == 0){
        ece391_fdputs (1, (uint8_t*)"input format: <music_num>\n");
        return -1;
    }

    play_music(music_num);

    return 0;
}
