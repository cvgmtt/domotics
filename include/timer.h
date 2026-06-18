#ifndef __TIMER_H__
#define __TIMER_H__
#include "return_code.h"
#include "device.h"

typedef struct registry_timer{
    int begin_minutes;
    int end_minutes;
    int id;
}registry_timer;

typedef struct timer{
    int state;
    int switches;
    registry_timer registry;
}timer;



timer createTimer();
int createProcessTimer(int num);

#endif