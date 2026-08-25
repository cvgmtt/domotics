#include "timer.h"

timer createTimer(){
    timer timer;
    timer.state = 1;
    timer.switches = 1;
    timer.registry.begin_time = 0;
    timer.registry.end_time = 0;
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
        
        int pipe = setup_device(timer.registry.id, "Timer", fd);
        if (pipe < 0) exit(FAILURE);
        char buf[MSG_SIZE];
        char msg[MSG_SIZE];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        char pipename_child[30];
        char pipename_parent[20];
        int child_pipe;

        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            char buf_copy[MSG_SIZE];
            memcpy(buf_copy, buf, MSG_SIZE);
            if(bytes_read > 0){
                command = getCommand(buf_copy, id, pos, child_id);
                char info[MSG_SIZE];
                memset(info, 0, sizeof(info));
                if (command != INVALID_COMMAND) {
                    wait_function();
                }
                switch (command)
                {
                    case CHANGE_PARENT_COMMAND:
                        timer.registry.child_id = -1;            
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                        if (child_pipe != -1) {
                            write(child_pipe, buf, sizeof(buf));
                            close(child_pipe);
                        } else{
                            printf("couldn't open the child pipe \n");
                        }
                        break;

                    case CHANGE_CHILD_COMMAND:
                        timer.registry.child_id = atoi(child_id);
                        if(timer.switches == 0){
                            snprintf(msg, sizeof(msg), "switch off");
                        } else{
                            snprintf(msg, sizeof(msg), "switch on");
                        }
                        send_ipc_message(child_id, msg);
                        break;

                    case SELF_INFO_COMMAND:
                        timer_info_command(&timer, info, sizeof(info));
                        send_info_to_controller(info);                  
                        break;

                    case CHILD_INFO_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", timer.registry.id);
                        child_info_command(pipename_child);
                        break;
                    
                    case SELF_DEL_COMMAND:
                        if (timer.registry.child_id != -1) {
                            int target_child = timer.registry.child_id;
                            
                            char pipename_child[32];
                            snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", target_child);
                            char msg[MSG_SIZE];
                            memset(msg, 0, sizeof(msg));
                            snprintf(msg, sizeof(msg), "self_delete %d", target_child);
                            
                            int target_pipe = open(pipename_child, O_WRONLY);
                            if (target_pipe != -1) {
                                write(target_pipe, msg, sizeof(msg));
                                close(target_pipe);
                                
                                char response[MSG_SIZE];
                                int result = wait_for_device_response(timer.registry.id, response, sizeof(response));
                                if (result == TIME_OUT) {
                                    printf("the child device %d didn't respond in time, assuming it's already terminated.\n", target_child);
                                } else if (result == PIPE_ERROR) {
                                    printf("error in opening timer pipe, moving on...\n");
                                }
                            } else {
                                printf("couldn't open the pipe of device %d, assuming it's already terminated.\n", target_child);
                            }
                            
                            timer.registry.child_id = -1;
                        }
                        
                        kill_device(timer.registry.id);
                        break;
                    case CHILD_DEL_COMMAND:
                        if (timer.registry.child_id != -1) {
                            snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", timer.registry.child_id);
                            char msg[MSG_SIZE];
                            memset(msg, 0, sizeof(msg));
                            snprintf(msg, sizeof(msg), "self_delete %d", timer.registry.child_id);
                            
                            int target_pipe = open(pipename_child, O_WRONLY);
                            if (target_pipe != -1) {
                                write(target_pipe, msg, sizeof(msg));
                                close(target_pipe);
                                
                                char response[MSG_SIZE];
                                int result = wait_for_device_response(timer.registry.id, response, sizeof(response));
                                if (result == SUCCESS) {
                                    printf("interaction device deleted successfully\n");
                                } else if (result == TIME_OUT){
                                    printf("couldn't delete child device\n");
                                    break;
                                } else if (result == PIPE_ERROR){
                                    printf("error in opening timer pipe to check for device response, assuming the message arrived and the interaction device was deleted\n");
                                }
                            } else {
                                printf("couldn't open the interaction device pipe of %d, assuming the device has crashed.\n", timer.registry.child_id);
                            }
                            
                            timer.registry.child_id = -1;
                        }
                        break;
                    case SWITCH_COMMAND:
                        //debug
                        printf("command switch reached in timer\n");
                        //changing state and switch
                        if (strcmp(pos, "on") == 0){
                            timer.switches = 1;
                            timer.state = 1;
                            timer.registry.begin_time = time(NULL);
                            timer.registry.end_time = 0;
                            printf("switched Timer %s\n", pos);
                        } else if (strcmp(pos, "off") == 0){
                            timer.switches = 0;
                            timer.state = 0;
                            timer.registry.end_time = time(NULL);
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
                        
                        break;
                    case SET_COMMAND:
                        memcpy(buf_copy, buf, MSG_SIZE);
                        strtok(buf_copy, " ");
                        char* attribute = strtok(NULL, " ");
                        char* value1 = strtok(NULL, " ");
                        char* value2 = strtok(NULL, " ");
                        if(strcmp(attribute, "timer") == 0){
                            if(atoi(value1) < atoi(value2)){
                                timer.registry.begin_time =  atoi(value1);
                                timer.registry.end_time = atoi(value2);
                            } else{
                                printf("invalid begin and end time given \n");
                            }
                            
                        } else{
                            printf("invalid attribute \n");
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
    if(check_inconsistency(current_timer->state, current_timer->switches)){
        printf("state inconsistency detected, manual interaction needed \n");
    }else{
        char registry[100];
        timer_registry_info(current_timer, registry, sizeof(registry));
        if(registry == NULL){
            printf("error reading registry \n");
            return;
        }
        //formats the info as "State: <state> Switch: <switches> Registry: <registry info>"
        snprintf(info, size,
            "State: %d Switch: %d, %s",
            current_timer->state,
            current_timer->switches,
            registry );
    }
}

//gets the info of the registry of the timer and returns it as a string
void timer_registry_info(timer* current_timer,  char* info, size_t size){
    snprintf(info, size,
        "Id=%d, Parent id: %d, Child id: %d, begin_time: %d, end_time: %d",
        current_timer->registry.id,
        current_timer->registry.parent_id,
        current_timer->registry.child_id,
        current_timer->registry.begin_time,
        current_timer->registry.end_time
    );
}
