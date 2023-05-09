#include <ucontext.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>


timer_t reconnect_timer_id;
timer_t timeout_timer_id;

int main(int argc, char **argv)
{

    makeTimer("Timeout Timer", &timeout_timer_id, 6,0);
    makeTimer("Timeout Timer", &reconnect_timer_id, 6,0);

}

void timerHandler(){
    printf("Timer expired\n");
}

makeTimer( char *name, timer_t *timerID, int expireSeconds, int intervalSeconds )
{
    struct sigevent         te;
    struct itimerspec       its;
    struct sigaction        sa;
    int sigNo = SIGRTMIN;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(sigNo, &sa, NULL) == -1) {
          printf("Failed to setup signal handling for");

        return(-1);
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    te.sigev_value.sival_ptr = timerID;
    timer_create(CLOCK_REALTIME, &te, timerID);
    its.it_interval.tv_sec = intervalSeconds;
    its.it_interval.tv_nsec =0;

    its.it_value.tv_sec = expireSeconds;
    its.it_value.tv_nsec = 0;
    timer_settime(*timerID, 0, &its, NULL);
    return(0);
}