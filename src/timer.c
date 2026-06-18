#include "timer.h"

timer createTimer(){
    timer timer;
    timer.state = 1;
    timer.switches = 1;
    timer.registry.begin_minutes = 0;
    timer.registry.end_minutes = 0;
    return timer;
}

int createProcessTimer(int num){
    timer timer = createTimer();
    int fd[2]; 
    if (pipe(fd) == -1) { 
        perror("could not open pipe"); return FAILURE; 
    }
    pid_t pid = fork();
    
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        FILE* fp = initDevice(fd);
        timer.registry.id = num + 1;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Timer, \n", timer.registry.id, child_pid_int);
        fclose(fp);
    
    
        while(1){
            
        }
    } else{
        return checkSuccess(fd, pid);
    }
}