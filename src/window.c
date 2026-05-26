#include "window.h"
#include "time.h"
#include "return_code.h"

window createWindow(){
    window window;
    window.state = 0;
    window.switches = 1;
    window.time_open = 0;
    return window;
}

int createProcessWindow(int num){
    window window = createWindow();
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
        window.id = num;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Window, \n", window.id, child_pid_int);
        fclose(fp);

        while(1){
            
        }
    }
}