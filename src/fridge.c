#include "fridge.h"

fridge createFridge(int _perc, int _temp, int _thermostat){
    fridge fridge;
    fridge.state = 0;
    fridge.switches = 1;
    fridge.registry.time_open = 0;
    fridge.registry.perc = _perc;
    fridge.registry.temp = _temp;
    fridge.registry.thermostat = _thermostat;
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
        fridge.registry.id = num + 1;
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Fridge, \n", fridge.registry.id, child_pid_int);
        fclose(fp);
    
        while(1){
            
        }
    }
}