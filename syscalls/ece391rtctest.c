//
// Created by liuzikai on 11/30/19.
//

#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define BUFSIZE 1024

int main ()
{
    uint32_t i, cnt, max = 0;
    int rtc_fd;
    int ret_val;
    int garbage;
    uint8_t buf[BUFSIZE];

    ece391_fdputs(1, (uint8_t*)"Enter the Test Number: (0): 50/2Hz, (1): 200/16Hz, (2): 25600/1024Hz\n");
    if (-1 == (cnt = ece391_read(0, buf, BUFSIZE-1)) ) {
        ece391_fdputs(1, (uint8_t*)"Can't read the number from keyboard.\n");
        return 3;
    }
    buf[cnt] = '\0';

    if ((ece391_strlen(buf) > 2) || ((ece391_strlen(buf) == 2) && ((buf[0] < '0') || (buf[0] > '2')))) {
        ece391_fdputs(1, (uint8_t*)"Wrong Choice!\n");
        return 0;
    } else {
        rtc_fd = ece391_open((uint8_t*)"rtc");
        switch (buf[0]) {
            case '0':
                max = 50;
                // Default 2Hz
                break;
            case '1':
                max = 200;
                ret_val = 16;
                ret_val = ece391_write(rtc_fd, &ret_val, 4);
                break;
            case '2':
                max = 25600;
                ret_val = 1024;
                ret_val = ece391_write(rtc_fd, &ret_val, 4);
                break;
        }
    }

    for (i = 0; i < max; i++) {
        ece391_itoa(i+1, buf, 10);
        ece391_fdputs(1, buf);
        ece391_fdputs(1, (uint8_t*)"\n");
        ece391_read(rtc_fd, &garbage, 4);
    }

    return 0;
}
