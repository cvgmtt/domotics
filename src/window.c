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
        window.registry.id = num + 1;
        int success = FAILURE;
        char pipename[20];
        do{
            success = createPipe(window.registry.id, pipename, sizeof(pipename));
        } while (success == FAILURE);

        int pipe = open(pipename, O_RDWR);
        FILE* fp = initDevice(fd, pipe);

        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Window, 0, \n", window.registry.id, child_pid_int);
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
                        window.registry.parent_id = atoi(id);
                        break;
                    case SELF_DEL_COMMAND:
                        if(window.registry.parent_id != 0){
                            char pipename_parent[20];
                            snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", window.registry.parent_id);
                            if(confirm_del(pipename_parent) == SUCCESS){
                                kill_device(window.registry.id);
                                break;
                            } else{
                                printf("error in deleting device with id %d", window.registry.id);
                                break;
                            }
                        }
                        kill_device(window.registry.id);
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