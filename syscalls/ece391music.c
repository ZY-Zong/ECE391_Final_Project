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


song_interval_t sound_startup_intel[] = {
            {Fa4H, 250}, {Fa4H, 250}, {Silent, 250}, {So5M, 250}, {Do1H, 250}, {So5M, 250}, {Re2H, 250}, {Re2H, 250},
            {Re2H, 250}
    };

song_interval_t sound_da_bu_zi_duo_ge[] = {
            //Music bar 1
            {Mi3M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150},
            {So5M, 300}, {Silent, 300}, {Do1M, 150}, {Silent, 150}, {Re2M, 300}, {Silent, 300},
            {Fa4M, 150}, {Silent, 150}, {Mi3M, 300}, {Silent, 300},
            //Music bar 2
            {So5M, 150}, {Silent, 150}, {La6M, 150}, {Silent, 150}, {So5M, 150}, {Silent, 150},
            {Fa4M, 150}, {Silent, 150}, {La6M, 300}, {Silent, 300}, {La6M, 150}, {Silent, 150},
            {Si7M, 150}, {Silent, 150}, {La6M, 150}, {Silent, 150}, {So5M, 300}, {Silent, 300},
            //Music bar 3
            {So5M, 150}, {Silent, 150}, {Do1H, 300}, {Silent, 300}, {Si7M, 100}, {Silent,  50},
            {La6M, 100}, {Silent, 150}, {So5M, 300}, {Silent, 300}, {Fa4M, 100}, {Silent,  50},
            {Mi3M, 100}, {Silent, 150}, {La6M, 150}, {Silent, 150}, {Re2M, 150}, {Silent, 150},
            {Mi3M, 150}, {Silent, 150}, {Re2M, 300}, {Silent, 300},
            //Music bar 4
            {Mi3M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150}, {La6M, 150}, {Silent, 150},
            {La6M, 300}, {Silent, 150}, {So5M, 150}, {Silent,  50}, {So5M, 150}, {Silent, 200},
            {So5M, 150}, {Silent, 150}, {So5M, 150}, {Silent, 150}, {Do1H, 150}, {Silent, 150},
            {Si7M, 300}, {Silent, 300},
            //Music bar 5
            {Si7M, 100}, {Silent,  50}, {Do1H, 100}, {Silent, 100}, {Re2H, 100}, {Silent,  50},
            {Do1H, 100}, {Silent,  50}, {Si7M, 100}, {Silent,  50}, {La6M, 100}, {Silent,  50},
            {So5M, 150}, {Silent, 150}, {Re2M, 200}, {Silent, 150}, {La6M, 200}, {Silent, 150},
            {Si7L, 200}, {Silent, 150}, {Do1M, 600}, {Silent, 600},
            //Music bar 6
            {Re2H, 400}, {Silent,  150}, {Re2H, 600}, {Silent, 600}, {Mi3H,600}, {Silent, 600},
            {Do1H, 1200}, {Silent, 150}
    };

song_interval_t sound_kong_fu_FC[] = {
            {La6H, 150}, {Silent, 150}, {La6H, 100}, {Silent, 50}, {La6H, 100}, {Silent, 100},
            {So5H, 150}, {Silent, 150}, {So5H, 150}, {Silent, 150},{Mi3H, 150}, {Silent, 150},
            {Mi3H, 150}, {Silent, 150}, {Do1H, 300}, {Silent, 300},

            {Re2H, 100}, {Silent,  50}, {Mi3H, 100}, {Silent,  50}, {Re2H, 100}, {Silent,  50},
            {Do1H, 100}, {Silent,  100},{La6M, 150}, {Silent,  150},{Do1H, 150}, {Silent,  150},
            {La6M, 300}, {Silent,  150}
    };

song_interval_t sound_little_star[] = {
            {Do1M, 150}, {Silent, 150}, {Do1M, 150}, {Silent, 150}, {So5M, 150}, {Silent, 150},
            {So5M, 150}, {Silent, 150}, {La6M, 150}, {Silent, 150}, {La6M, 150}, {Silent, 150},
            {So5M, 150}, {Silent, 150}, {Silent, 150},
            {Fa4M, 150}, {Silent, 150}, {Fa4M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150},
            {Mi3M, 150}, {Silent, 150}, {Re2M, 150}, {Silent, 150}, {Re2M, 150}, {Silent, 150},
            {Do1M, 150}, {Silent, 150}, {Silent, 150},
            {So5M, 150}, {Silent, 150}, {So5M, 150}, {Silent, 150}, {Fa4M, 150}, {Silent, 150},
            {Fa4M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150},
            {Re2M, 150}, {Silent, 150}, {Silent, 150},
            {So5M, 150}, {Silent, 150}, {So5M, 150}, {Silent, 150}, {Fa4M, 150}, {Silent, 150},
            {Fa4M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150},
            {Re2M, 150}, {Silent, 150}, {Silent, 150},
            {Do1M, 150}, {Silent, 150}, {Do1M, 150}, {Silent, 150}, {So5M, 150}, {Silent, 150},
            {So5M, 150}, {Silent, 150}, {La6M, 150}, {Silent, 150}, {La6M, 150}, {Silent, 150},
            {So5M, 150}, {Silent, 150}, {Silent, 150},
            {Fa4M, 150}, {Silent, 150}, {Fa4M, 150}, {Silent, 150}, {Mi3M, 150}, {Silent, 150},
            {Mi3M, 150}, {Silent, 150}, {Re2M, 150}, {Silent, 150}, {Re2M, 150}, {Silent, 150},
            {Do1M, 150}, {Silent, 150}, {Silent, 150}
    };

song_interval_t sound_orange[] = {
            { 880, 1230}, {   0,   66}, {1174,  204}, {   0,   12}, {1046,  410}, {   0,   22}, { 932,  410},
            {   0,   22}, { 880,  410}, {   0,   22}, { 698,  204}, {   0,   12}, { 783,  204}, {   0,  228},
            { 698,  820}, {   0,   44}, { 659,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 783,  204},
            {   0,   12}, { 880,  410}, {   0,   23}, { 783,  410}, {   0,   23}, { 698,  410}, {   0,   23},
            { 783,  410}, {   0,   23}, { 880,  204}, {   0,   12}, {1046, 1230}, {   0,   66}, {1174,  204},
            {   0,   12}, {1046,  410}, {   0,   23}, { 932,  410}, {   0,   23}, { 880,  410}, {   0,   23},
            { 698,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 523,  204}, {   0,   12}, { 698,  820},
            {   0,   44}, { 659,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 659,  204}, {   0,   12},
            { 587, 1846}, {   0,  530}, { 880,  204}, {   0,   12}, { 932,  204}, {   0,   12}, { 880,  204},
            {   0,   12}, { 698,  615}, {   0,   33}, { 880,  204}, {   0,   12}, { 932,  204}, {   0,   12},
            { 880,  204}, {   0,   12}, { 698,  820}, {   0,  692}, { 880,  204}, {   0,   12}, { 932,  204},
            {   0,   12}, { 880,  204}, {   0,   12}, { 698,  409}, {   0,   22}, { 698,  204}, {   0,   12},
            { 880,  204}, {   0,   12}, {1046,  409}, {   0,   22}, {1046, 1025}, {   0,  487}, { 880,  204},
            {   0,   12}, { 932,  204}, {   0,   12}, { 880,  204}, {   0,   12}, { 698,  615}, {   0,   33},
            { 880,  204}, {   0,   12}, { 932,  204}, {   0,   12}, { 880,  204}, {   0,   12}, { 698,  820},
            {   0,  692}, { 880,  204}, {   0,   12}, { 932,  204}, {   0,   12}, { 880,  204}, {   0,   12},
            { 698,  409}, {   0,   22}, { 698,  204}, {   0,   12}, {1046,  215}, {   0,  217}, { 932,  215},
            {   0,  217}, { 880,  215}, {   0,  217}, { 783,  215}, {   0,  649}, { 587,  204}, {   0,   12},
            { 698,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 783,  410}, {   0,   22}, { 587,  409},
            {   0,   22}, { 698,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 783,  204}, {   0, 1308},
            { 587,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 783,  409},
            {   0,   22}, { 880,  409}, {   0,   22}, { 698,  204}, {   0,   12}, { 698,  204}, {   0,   12},
            { 698,  204}, {   0, 1308}, { 587,  409}, {   0,   22}, { 698,  204}, {   0,   12}, { 783,  107},
            {   0,  325}, { 587,  409}, {   0,   22}, { 698,  107}, {   0,  325}, { 659,  409}, { 783,  107},
            {   0,  541}, { 880,  204}, {   0,   12}, { 698, 1435}, {   0,   77}, { 880,  204}, {   0,   12},
            { 783, 1435}, {   0,   77}, {1046,  409}, {   0,   22}, {1046,  820}, {   0,   44}, { 932,  204},
            {   0,   12}, { 880,  409}, {   0,   22}, { 783, 1435}, {   0,  941}, { 698,  409}, {   0,   22},
            { 783,  409}, {   0,   22}, { 698,  409}, {   0,   22}, { 880,  204}, {   0,   12}, { 880,  204},
            {   0,   12}, { 783,  204}, {   0,   12}, { 880,  204}, {   0,  228}, { 880,  204}, {   0,   12},
            { 783,  204}, {   0,   12}, { 880,  204}, {   0,  228}, { 880,  204}, {   0,   12}, { 783,  204},
            {   0,   12}, { 880,  204}, {   0,   12}, {1046,  204}, {   0,   12}, { 880,  204}, {   0,   12},
            { 783,  204}, {   0,   12}, { 659,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 783,  204},
            {   0,   12}, { 698,  204}, {   0,   12}, { 783,  204}, {   0,  228}, { 783,  204}, {   0,   12},
            { 698,  204}, {   0,   12}, { 783,  204}, {   0,  228}, { 880,  204}, {   0,   12}, { 932,  204},
            {   0,   12}, { 880,  409}, {   0,   22}, { 698,  204}, {   0,   12}, { 783,  204}, {   0,   12},
            { 698,  204}, {   0,   12}, { 880,  204}, {   0,   12}, { 880,  204}, {   0,   12}, { 783,  204},
            {   0,   12}, { 880,  204}, {   0,  228}, { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12},
            { 880,  204}, {   0,  228}, { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 880,  204},
            {   0,   12}, {1046,  204}, {   0,   12}, { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12},
            { 659,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 698,  204},
            {   0,   12}, { 783,  204}, {   0,  228}, {1046,  409}, {   0,   23}, {1046,  615}, {   0, 1329},
            { 880,  204}, {   0,   12}, { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 880,  204},
            {   0,  228}, { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 880,  204}, {   0,  228},
            { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 880,  204}, {   0,   12}, {1046,  204},
            {   0,   12}, { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 698,  204}, {   0,   12},
            { 783,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 783,  204},
            {   0,  228}, { 783,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 783,  204}, {   0,  228},
            { 880,  204}, {   0,   12}, { 932,  204}, {   0,   12}, { 880,  409}, {   0,   23}, { 698,  204},
            {   0,   12}, { 783,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 880,  204}, {   0,   12},
            { 880,  204}, {   0,   12}, { 783,  204}, {   0,   12}, { 880,  204}, {   0,  228}, { 880,  204},
            {   0,   12}, { 783,  204}, {   0,   12}, { 880,  204}, {   0,  228}, { 880,  204}, {   0,   12},
            { 783,  204}, {   0,   12}, { 880,  204}, {   0,   12}, {1046,  204}, {   0,   12}, { 880,  204},
            {   0,   12}, { 783,  204}, {   0,   12}, { 659,  204}, {   0,   12}, { 783,  204}, {   0,   12},
            { 783,  204}, {   0,   12}, { 698,  204}, {   0,   12}, { 783,  204}, {   0,  228}, {1046,  409},
            {   0,   23}, {1046, 1025}
    };


/**
 * Beep
 * @param   frequency: the frequency of beep 
 * @param   time_in_ms: the duration of the beep 
 * @effect: the frequency of PIT channel 2 will be changed!
 */
void beep(uint32_t frequency, uint32_t time_in_ms){
    
    if (time_in_ms == 0) return;
    
    // Open a rtc and set frequency
    int i; // loop counter
    uint8_t buf[]="rtc";
    uint32_t t = 256;
    int rtc_fd=ece391_open(buf); // default frequency 2Hz
    ece391_write(rtc_fd, &t, 4);
    
    if (rtc_fd == -1) return;
    
    ece391_playsound(frequency); // turn on the sound 

    // beeeeeeeeee
    for (i = 0; i< time_in_ms; i++) ece391_read(rtc_fd, 0, 0);
    
    ece391_nosound(); // turn off the sound 

    ece391_close(rtc_fd);
}

void play_music_with_pu(song_interval_t* pu, int32_t length){
    int i=0; // loop counter 
    for (i=0; i<length; i++){
        beep(pu[i].frequency, pu[i].time_in_half_sec);
    }
}


void play_music(music_num){
    switch (music_num){
        case 0:
            play_music_with_pu(sound_startup_intel, 9);
            break;
        case 1: 
            beep(Do1H, 100);
            beep(Re2H, 100);
            beep(Mi3H, 100);
            beep(Fa4H, 100);
            beep(So5H, 100);
            beep(Fa4H, 100);
            beep(Mi3H, 100);
            beep(Re2H, 100);
            beep(Do1H, 100);
            break;
        case 2: 
            beep(Do1H, 100);
            beep(Do1H, 100);
            beep(So5H, 100);
            beep(So5H, 100);
            beep(La6H, 100);
            beep(La6H, 100);
            beep(So5H, 100);
            beep(0,100);
            beep(Fa4H, 100);
            beep(Fa4H, 100);
            beep(Mi3H, 100);
            beep(Mi3H, 100);
            beep(Re2H, 100);
            beep(Re2H, 100);
            beep(Do1H, 100);
            break;
        case 3: 
            play_music_with_pu(sound_da_bu_zi_duo_ge, 106);
            break;
        case 4: 
            play_music_with_pu(sound_kong_fu_FC, 30);
            break;
        case 5: 
            play_music_with_pu(sound_little_star, 90);
            break;
        case 6: 
            play_music_with_pu(sound_orange, 380);
            break;
        default: 
            beep(Do1H, 100);
            beep(Re2H, 100);
            beep(Mi3H, 100);
            beep(Fa4H, 100);
            beep(So5H, 100);
            beep(Fa4H, 100);
            beep(Mi3H, 100);
            beep(Re2H, 100);
            beep(Do1H, 100);
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
