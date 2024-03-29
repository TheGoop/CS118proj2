/* 
From Tianyuan Yu's Week 7 slides
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <iostream>
#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

void errExit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

static void handler(union sigval val){
   std::cout << "callback triggered: " << val.sival_int << std::endl;
}

int main(int argc, char *argv[]){
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    long secs;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <timeout>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    /* Create the timer */ 
    // 3 elements: ID, timeout value, callback
    union sigval arg;
    arg.sival_int = 54321;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = handler;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value = arg;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
        errExit("timer_create");
    /* Start the timer */
    secs = atoll(argv[1]);
    its.it_value.tv_sec = secs;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = secs;
    its.it_interval.tv_nsec = 0;

    // struct itimerspec its;
    // long long freq_nanosecs = 200 * 1000000;
    // its.it_value.tv_sec = freq_nanosecs / 1000000000; // First expiration
    // its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    // its.it_interval.tv_sec = its.it_value.tv_sec; // Periodic expiration
    // its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1)
        errExit("timer_settime");
    while(getchar() =='\n') {
        // disarm the timer
        its = {{0, 0}, {0, 0}};
        if (timer_settime(timerid, 0, &its, NULL) == -1)
            errExit("timer_settime");
        std::cout << "disarm the timer" << std::endl;
    };
    exit(EXIT_SUCCESS);
}