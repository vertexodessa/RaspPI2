/*********************************************************************
 * Implement a precision "timed wait".  The parameter early_usec
 * allows an interrupted select(2) call to consider the wait as
 * completed, when interrupted with only "early_usec" left remaining.
 *********************************************************************/

#include "pi_utils.h"

#include <assert.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/poll.h>


namespace PI {

double measure_dist(UsualGPIO* trig, UsualGPIO* echo) {
    trig->WriteLow();
    timed_wait(0,100000);
    trig->WriteHigh();
    timed_wait(0,5);
    trig->WriteLow();


    struct timespec start, stop;
    while(!echo->Read()){}
    clock_gettime(CLOCK_REALTIME, &start);

    while(echo->Read()){}
    clock_gettime(CLOCK_REALTIME, &stop);

    timespec diff = time_diff(start, stop);

    double dist = diff.tv_nsec / 58000.0;
    return dist;
}

void timed_wait(long sec,long usec,long early_usec) {
    fd_set mt;
    struct timeval timeout;
    int rc;

    FD_ZERO(&mt);
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;
    do  {
        rc = select(0,&mt,&mt,&mt,&timeout);
        if ( !timeout.tv_sec && timeout.tv_usec < early_usec )
            return;     /* Wait is good enough, exit */
    } while ( rc < 0 && timeout.tv_sec && timeout.tv_usec );
}
}

/*********************************************************************
 * End pi_utils.c
 * Mastering the Raspberry Pi - ISBN13: 978-1-484201-82-4
 * This source code is placed into the public domain.
 *********************************************************************/
