#include "timer.h"

timer createTimer(){
    timer timer;
    timer.state = 1;
    timer.switches = 1;
    timer.registry.begin_minutes = 0;
    timer.registry.end_minutes = 0;
    timer.registry.parent_id = 0;
    timer.registry.child_id = -1;
    return timer;
}

int createProcessTimer(int num){
    timer timer = createTimer();
    int fd[2]; 
    if (pipe(fd) == -1) { 
        perror("could not open pipe"); return FAILURE; 
    }
    pid_t pid = fork();
    
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        timer.registry.id = num + 1;
        int success = FAILURE;    
        char pipename[20];
        do{
            success = createPipe(timer.registry.id, pipename, sizeof(pipename));
        } while (success == FAILURE);

        int pipe = open(pipename, O_RDWR);
        FILE* fp = initDevice(fd, pipe);
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Timer, 0, \n", timer.registry.id, child_pid_int);
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
                switch (command)
                {
                    case CHANGE_PARENT_COMMAND:
                        timer.registry.child_id = -1;            
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                        write(child_pipe, buf, sizeof(buf));
                        close(child_pipe);
                        break;

                    case CHANGE_CHILD_COMMAND:
                        timer.registry.child_id = atoi(id);
                        break;
                    
                    case SELF_DEL_COMMAND:
                        if(timer.registry.child_id != -1){
                            //send the delete command to its child
                            snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", timer.registry.child_id);
                            char msg[20];
                            snprintf(msg, sizeof(msg), "self_delete %d", timer.registry.child_id);
                            int pipe = open(pipename_child, O_WRONLY);
                            if(pipe != -1){
                                write(pipe, msg, sizeof(msg));
                                close(pipe);
                            } else{
                                printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                                break;
                            }
                            //checks whether the child has received to delete command
                            char pipename[20];
                            snprintf(pipename, sizeof(pipename), "/tmp/domotics_%d", timer.registry.id);
                            pipe = open(pipename, O_RDONLY | O_NONBLOCK);
                            if(pipe != -1){
                                fd_set read_fds;
                                FD_ZERO(&read_fds);
                                FD_SET(pipe, &read_fds);

                                //max timeout
                                struct timeval tv;
                                tv.tv_sec = 1;
                                tv.tv_usec = 0;

                                int activity = select(pipe + 1, &read_fds, NULL, NULL, &tv);

                                if(activity > 0){
                                    bytes_read = read(pipe, msg, sizeof(msg));
                                    if (bytes_read > 0){
                                        close(pipe);
                                        kill_device(timer.registry.id);
                                        break;
                                    } else{
                                        close(pipe);
                                        printf("couldn't delete child device, therefore not deleting control device");
                                        break;
                                    }
                                } else if (activity == 0){
                                    close(pipe);
                                    printf("couldn't delete child device, therefore not deleting control device");
                                    break;
                                } else{
                                    close(pipe);
                                    break;
                                }
                            } else{
                                printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                                break;
                            }
                        } else{
                            //if no children kills it immediatly
                            kill_device(timer.registry.id);
                            break;
                        }
                    case CHILD_DEL_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", timer.registry.child_id);
                        char msg[20];
                        snprintf(msg, sizeof(msg), "self_delete %d", timer.registry.child_id);
                        int pipe = open(pipename_child, O_WRONLY);
                        if(pipe != -1){
                            write(pipe, msg, sizeof(msg));
                            close(pipe);
                        } else{
                            printf("couldn't open the pipe of the interaction device to delete it");
                            break;
                        }
                        //checks
                        char pipename[20];
                        snprintf(pipename, sizeof(pipename), "/tmp/domotics_%d", timer.registry.id);
                        pipe = open(pipename, O_RDONLY | O_NONBLOCK);
                        if(pipe != -1){
                            fd_set read_fds;
                            FD_ZERO(&read_fds);
                            FD_SET(pipe, &read_fds);

                            //max timeout
                            struct timeval tv;
                            tv.tv_sec = 1;
                            tv.tv_usec = 0;

                            int activity = select(pipe + 1, &read_fds, NULL, NULL, &tv);

                            if(activity > 0){
                                // interaction device has responded in time
                                bytes_read = read(pipe, msg, sizeof(msg));
                                if (bytes_read > 0){
                                    close(pipe);
                                    timer.registry.child_id = -1;
                                }
                                printf("interaction device deleted successfully");  
                                break;
                            } else if (activity == 0){
                                close(pipe);
                                printf("couldn't delete child device, therefore not deleting control device");
                                break;
                            } else{
                                close(pipe);
                            }
                        } else{
                            printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                            timer.registry.child_id = -1;
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