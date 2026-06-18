#include "hub.h"

hub createHub(){
    hub hub;
    hub.state = 1; //capisci se va bene
    hub.switches = 1;
    hub.registry.child_switches[5];
    return hub;
}

int createProcessHub(int num){
    hub hub = createHub();
    int fd[2]; // fd[0] is the read end, fd[1] is the write end
    if (pipe(fd) == -1) { 
        perror("could not open pipe"); return FAILURE; 
    }
    pid_t pid = fork();
    
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        FILE* fp = initDevice(fd);
        hub.registry.id = num +1;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Hub, \n", hub.registry.id, child_pid_int);
        fclose(fp);
        
    
        while(1){
            
        }
    } else{
        return checkSuccess(fd, pid);
    }
}