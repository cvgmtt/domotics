#include "fridge.h"

fridge createFridge(int _perc, int _temp, int _thermostat){
    fridge fridge;
    fridge.state = 0;
    fridge.switches = 1;
    fridge.registry.time_open = 0;
    fridge.registry.perc = _perc;
    fridge.registry.temp = _temp;
    fridge.registry.thermostat = _thermostat;
    fridge.registry.parent_id = 0;
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

        int pipe = open(pipename, O_RDWR);
        FILE* fp = initDevice(fd, pipe);
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Fridge, 0, \n", fridge.registry.id, child_pid_int);
        fclose(fp);
        char buf[50];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            if(bytes_read > 0){
                command = getCommand(buf, id, pos, child_id);

                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        fridge.registry.parent_id = atoi(id);
                        break;
                    case SELF_DEL_COMMAND:
                        if(fridge.registry.parent_id != 0){
                            char pipename_parent[20];
                            snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", fridge.registry.parent_id);
                            if(confirm_del(pipename_parent) == SUCCESS){
                                kill_device(fridge.registry.id);
                                break;
                            } else{
                                printf("error in deleting device with id %d", fridge.registry.id);
                                break;
                            }
                        }
                        kill_device(fridge.registry.id);
                        break;

                    default:
                        break;
                    }
            }
        }
    } else{
        return checkSuccess(fd, pid);
    }
}