#include "fridge.h"

fridge createFridge(int _perc, int _temp, int _thermostat){
    fridge fridge;
    fridge.state = 0;
    fridge.switches = 1;
    fridge.time_open = 0;
    fridge.perc = _perc;
    fridge.temp = _temp;
    fridge.thermostat = _thermostat;
    return fridge;

}

int createProcessFridge(int num){
    fridge fridge = createFridge(0, 0, 0);
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
        fridge.id = num;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Fridge, \n", fridge.id, child_pid_int);
        fclose(fp);
    
        while(1){
            
        }
    }
}