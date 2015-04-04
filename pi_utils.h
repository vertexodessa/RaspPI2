#ifndef PI_UTILS
#define PI_UTILS

#include "pi_gpio.h"

namespace PI {
void timed_wait(long sec,long usec,long early_usec =0);

double measure_dist(UsualGPIO* trig, UsualGPIO* echo);

inline timespec time_diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
};

}
#endif // PI_UTILS

