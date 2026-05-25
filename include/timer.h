#ifndef __TIMER_H__
#define __TIMER_H__
#include "return_code.h"

typedef struct timer{
    int state;
    int switches;
    int begin_minutes;
    int end_minutes;


}timer;

timer createTimer();
int createProcessTimer();

#endif