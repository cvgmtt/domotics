#include "window.h"
#include "time.h"
#include "return_code.h"

window createWindow(){
    window window;
    window.state = 0;
    window.switches = 1;
    window.registry.time_open = 0;
    return window;
}

int createProcessWindow(int num){
    window window = createWindow();
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
        window.registry.id = num + 1;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Window, \n", window.registry.id, child_pid_int);
        fclose(fp);

        while(1){
            
        }
    } else{
        return checkSuccess(fd, pid);
    }
}