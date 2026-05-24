#include "timer.h"

timer createTimer(){
    timer timer;
    timer.state = 1;
    timer.switches = 1;
    timer.begin_minutes = 0;
    timer.end_minutes = 0;
    return timer;
}