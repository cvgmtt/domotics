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
        char pipename_child[30];
        char controller_pipename[20];
        char pipename_parent[20];
        int child_pipe;
        snprintf(controller_pipename, sizeof(controller_pipename), "/tmp/domotics_0");

        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            if(bytes_read > 0){
                command = getCommand(buf, id, pos, child_id);
                char info[100];
                switch (command)
                {
                    case CHANGE_PARENT_COMMAND:
                        timer.registry.child_id = -1;            
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                        write(child_pipe, buf, sizeof(buf));
                        close(child_pipe);
                        break;

                    case CHANGE_CHILD_COMMAND:
                        timer.registry.child_id = atoi(child_id);
                        break;

                    case SELF_INFO_COMMAND:
                        timer_info_command(&timer, info, sizeof(info));
                        if(strcmp(info, "") != 0){
                            int controller_pipe = open(controller_pipename, O_WRONLY);
                            if (controller_pipe < 0) {
                                perror("open controller pipe");
                                break;
                            }
                            if (write(controller_pipe, info, strlen(info) + 1) < 0) {
                                perror("write controller pipe");
                            }
                            close(controller_pipe);
                        }                        
                        break;

                    case CHILD_INFO_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", timer.registry.id);
                        child_info_command(pipename_child, pipename_parent, info, sizeof(info));
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
                    case SWITCH_COMMAND:
                        //debug
                        printf("command switch reached in timer\n");
                        //changing state and switch
                        if (strcmp(pos, "on") == 0){
                            timer.switches = 1;
                            timer.state = 1;
                            timer.registry.begin_minutes = 0;
                            timer.registry.end_minutes = 0;
                            printf("switched Timer %s\n", pos);
                        } else if (strcmp(pos, "off") == 0){
                            timer.switches = 0;
                            timer.state = 0;
                            timer.registry.end_minutes = timer.registry.end_minutes;
                            timer.registry.begin_minutes = 0;

                            printf("switched Timer%s\n", pos);
                        }
                        //updating child
                        
                        if(timer.registry.child_id > 0){
                            snprintf(pipename_child, sizeof(pipename_child), "/t,mp/domotics_%d", timer.registry.child_id );
                            child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                            if (child_pipe >= 0){
                                char message[30];
                                snprintf(message, sizeof(message), "switch %s", pos);
                                if (write(child_pipe, message, strlen(message) + 1) < 0) {
                                    perror("write child pipe");
                                }
                                close(child_pipe);
                            }
                        }
                        //manca l'updating dell'array child_switches
                        //lo implemento dopo quando ho un sistema di notifica da parte del figlio che funziona
                        break;
                    case SWITCH_CHILD_COMMAND:
                        printf("command switch_child reached in timer\n");
                        char message[50];
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", id );
                        child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                        if (child_pipe >= 0){
                            snprintf(message, sizeof(message), "switch %s", pos);
                            if (write(child_pipe, message, strlen(message) + 1) < 0) {
                                perror("write child pipe");
                            }
                            close(child_pipe);
                        }
                        
                        //manca l'updating dell'array child_switches
                        //lo implemento dopo quando ho un sistema di notifica da parte del figlio che funziona
                        break;
                    case STATE_CHANGE:
                        int child_state = atoi(pos);
                        if(child_state != timer.state){
                            printf("timer's state is inconsistent");
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

//gets the info of the timer and returns it as a string
void timer_info_command(timer* current_timer, char* info, size_t size){
    char registry[100];
    timer_registry_info(current_timer, registry, sizeof(registry));
    //if the string of registry info is NULL, make it contain an error message
    if(strcmp(registry, "\0") == 0){
        perror("error reading registry");
        return;
    }
    //formats the info as "State: <state> Switch: <switches> Registry: <registry info>"
    snprintf(info, size,
        "State: %d Switch: %d Registry: %s",
        current_timer->state,
        current_timer->switches,
        registry );
}

//gets the info of the registry of the timer and returns it as a string
void timer_registry_info(timer* current_timer,  char* info, size_t size){
    char childs[100];
    strcpy(info, "\0");
    childs[0] = '\0';
    int len = 0;
    if(current_timer->registry.child_id == -1){
        snprintf(childs, sizeof(childs), "0");
    } else{
        snprintf(childs, sizeof(childs), "1");        
    }
    
    //formats the info as "id=<id> parent_id=<parent_id> child_num=<child_num> children=[<string of children>]"
    snprintf(info, size - len,
        "id=%d parent_id=%d child_num=%s children=[%d]",
        current_timer->registry.id,
        current_timer->registry.parent_id,
        childs,
        current_timer->registry.child_id
    );
}
