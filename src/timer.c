#include "timer.h"

timer createTimer(){
    timer timer;
    timer.state = 1;
    timer.switches = 1;
    timer.begin_minutes = 0;
    timer.end_minutes = 0;
    return timer;
}

int createProcessTimer(int num){
    timer timer = createTimer();
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        FILE* fp = fopen(".registry.txt", "a");
        if(fp == NULL){
            printf("could not open file");
            return FAILURE;
        }
        timer.id = num;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Timer, \n", timer.id, child_pid_int);
        fclose(fp);
    
        while(1){
            
        }
    }
}