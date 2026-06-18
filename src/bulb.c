#include "bulb.h"

bulb createBulb(){
    bulb bulb;
    bulb.state = 0;
    bulb.switches = 1;
    bulb.registry.time = 0.0;
    return bulb;
}

int createProcessBulb(int num){
    bulb bulb = createBulb();
    int fd[2]; 
    if (pipe(fd) == -1) { 
        perror("could not open pipe"); return FAILURE; 
    }
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    };
    
    if(pid == 0){
        FILE* fp = initDevice(fd);
        bulb.registry.id = num + 1;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Bulb, \n", bulb.registry.id, child_pid_int);
        fclose(fp);
  

        while(1){
            
        }
    } else{
        return checkSuccess(fd, pid);
    }
}