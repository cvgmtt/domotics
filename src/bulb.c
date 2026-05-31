#include "bulb.h"

bulb createBulb(){
    bulb bulb;
    bulb.state = 0;
    bulb.switches = 1;
    bulb.time = 0.0;
    return bulb;
}

int createProcessBulb(int num){
    bulb bulb = createBulb();
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    };
    
    if(pid == 0){
        FILE* fp = fopen(".registry.txt", "a");
        if(fp == NULL){
            printf("could not open file");
            return FAILURE;
        }
        bulb.id = num + 1;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Bulb, \n", bulb.id, child_pid_int);
        fclose(fp);
  

        while(1){
            
        }
    }
}