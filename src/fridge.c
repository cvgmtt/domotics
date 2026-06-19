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
    int fd[2]; 
    if (pipe(fd) == -1) { 
        perror("could not open pipe"); return FAILURE; 
    }
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        fridge.registry.id = num + 1;
        int success = FAILURE;        
        char pipename[20];
        do{
            success = createPipe(fridge.registry.id, pipename, sizeof(pipename));
        } while (success == FAILURE);

        int pipe = open(pipename, O_RDONLY | O_NONBLOCK);
        FILE* fp = initDevice(fd, pipe);
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Fridge, 0 %d\n", fridge.registry.id, child_pid_int, 0 );
        fclose(fp);

        while(1){

        }
    } else{
        return checkSuccess(fd, pid);
    }
}