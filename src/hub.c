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

        int pipe = open(pipename, O_RDWR);
  


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
                        for(int i = 0; i < 20; i++){
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
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] == -1){
                                hub.registry.child_id[i] = atoi(id);
                                hub.registry.child_switches[i] = hub.switches;
                                hub.registry.child_num++;
                                break;
                            }
                        }
                        break;
                    
                    case SELF_DEL_COMMAND:
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] != -1){
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", hub.registry.child_id[i]);
                                char msg[20];
                                snprintf(msg, sizeof(msg), "self_delete %d", hub.registry.child_id[i]);
                                int out_pipe = open(pipename_child, O_WRONLY);
                                if(out_pipe != -1){
                                    write(out_pipe, msg, sizeof(msg));
                                    close(out_pipe);
                                } else{
                                    printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                                    continue;
                                }
                                
                                char check_pipename[20];
                                snprintf(check_pipename, sizeof(check_pipename), "/tmp/domotics_%d", hub.registry.id);
                                int check_pipe = open(check_pipename, O_RDONLY | O_NONBLOCK);
                                if(check_pipe != -1){
                                    fd_set read_fds;
                                    FD_ZERO(&read_fds);
                                    FD_SET(check_pipe, &read_fds);

                                    //max timeout
                                    struct timeval tv;
                                    tv.tv_sec = 1;
                                    tv.tv_usec = 0;

                                    int activity = select(check_pipe + 1, &read_fds, NULL, NULL, &tv);

                                    if(activity > 0){
                                        bytes_read = read(check_pipe, msg, sizeof(msg));
                                        if (bytes_read > 0){
                                            close(check_pipe);
                                            hub.registry.child_id[i] = -1;
                                            hub.registry.child_switches[i] = -1;
                                            hub.registry.child_num--;
                                        } else{
                                            close(check_pipe);
                                        }
                                    } else if (activity == 0){
                                        close(check_pipe);
                                        printf("couldn't delete child device, therefore not deleting control device");
                                    } else{
                                        close(check_pipe);
                                    }
                                } else{
                                    printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                                }
                            }
                        }
                        
                        //if no children kills it immediatly
                        if(hub.registry.child_num == 0){
                            kill_device(hub.registry.id);
                            break;
                        } else{
                            printf("couldn't delete all child devices, therefore not deleting control device");
                            break;
                        }

                    case CHILD_DEL_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        char msg[20];
                        snprintf(msg, sizeof(msg), "self_delete %s", child_id);
                        int out_pipe = open(pipename_child, O_WRONLY);
                        if(out_pipe != -1){
                            write(out_pipe, msg, sizeof(msg));
                            close(out_pipe);
                        } else{
                            printf("couldn't open the pipe of the interaction device to delete it");
                            break;
                        }
                        //checks
                        char check_pipename[20];
                        snprintf(check_pipename, sizeof(check_pipename), "/tmp/domotics_%d", hub.registry.id);
                        int check_pipe = open(check_pipename, O_RDONLY | O_NONBLOCK);
                        if(check_pipe != -1){
                            fd_set read_fds;
                            FD_ZERO(&read_fds);
                            FD_SET(check_pipe, &read_fds);

                            //max timeout
                            struct timeval tv;
                            tv.tv_sec = 1;
                            tv.tv_usec = 0;

                            int activity = select(check_pipe + 1, &read_fds, NULL, NULL, &tv);

                            if(activity > 0){
                                // interaction device has responded in time
                                bytes_read = read(check_pipe, msg, sizeof(msg));
                                if (bytes_read > 0){
                                    close(check_pipe);
                                    for(int i = 0; i < 20; i++){
                                        if(hub.registry.child_id[i] == atoi(child_id)){
                                            hub.registry.child_id[i] = -1;
                                            hub.registry.child_switches[i] = -1;
                                            hub.registry.child_num--;
                                            break;
                                        }
                                    }
                                }
                                printf("interaction device deleted successfully");  
                                break;
                            } else if (activity == 0){
                                close(check_pipe);
                                printf("couldn't delete child device, therefore not deleting control device");
                                break;
                            } else{
                                close(check_pipe);
                            }
                        } else{
                            printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                            break;
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