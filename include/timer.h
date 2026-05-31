#ifndef __TIMER_H__
#define __TIMER_H__
#include "return_code.h"

typedef struct timer{
    int state;
    int switches;
    registry registry;
}timer;

typedef struct registry{
    int begin_minutes;
    int end_minutes;
    int id;
}registry;

timer createTimer();
int createProcessTimer(int num);

#endif