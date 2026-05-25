#ifndef __TIMER_H__
#define __TIMER_H__

typedef struct timer{
    int state;
    int switches;
    int begin_minutes;
    int end_minutes;


}timer;

timer createTimer();
int createProcess();

#endif