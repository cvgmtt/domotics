#include "timer.h"

timer createTimer(){
    timer timer;
    timer.state = 1;
    timer.switches = 1;
    timer.begin_minutes = 0;
    timer.end_minutes = 0;
    return timer;
}

int createProcessTimer(){
    timer timer = createTimer();
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    } else if(pid == 0){
        while(1){
            
        }
    }
}