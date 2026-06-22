#include "hub.h"

hub createHub(){
    hub hub;
    hub.state = 1; //capisci se va bene
    hub.switches = 1;
    hub.registry.child_num = 0;
    hub.registry.parent_id = 0;
    for(int i = 0; i < 20; i++){
        hub.registry.child_id[i] = -1;
        hub.registry.child_switches[i] = -1;
    }
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
        hub.registry.id = num +1;
        char pipename[20];
        int success = FAILURE;        
        do{
            success = createPipe(hub.registry.id, pipename, sizeof(pipename));
        } while (success == FAILURE);

        int pipe = open(pipename, O_RDONLY | O_NONBLOCK);
  

        FILE* fp = initDevice(fd, pipe);
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Hub, 0, \n", hub.registry.id, child_pid_int);
        fclose(fp);
        char buf[50];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        char pipename_child[20];
        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            if(bytes_read > 0){
                command = getCommand(buf, id, pos, child_id);

                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        for(int i = 0; i < hub.registry.child_num; i++){
                            if(hub.registry.child_id[i] == atoi(child_id)){
                                hub.registry.child_id[i] = -1;
                                hub.registry.child_switches[i] = -1;
                                hub.registry.child_num--;
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                                int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                                write(child_pipe, buf, sizeof(buf));
                                close(child_pipe);
                                break;
                            }
                        }
                        break;


                    case CHANGE_CHILD_COMMAND:
                        if (hub.registry.child_num == 20){

                        }
                        for(int i = 0; i < hub.registry.child_num; i++){
                            if(hub.registry.child_id[i] == -1){
                                hub.registry.child_id[hub.registry.child_num] = atoi(id);
                                hub.registry.child_switches[i] = hub.switches;
                                hub.registry.child_num++;
                                break;
                            }
                        }
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