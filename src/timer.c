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

                    case SELF_INFO_COMMAND:
                        char* info = self_info_command(&timer);
                        write(controller_pipename, info, strlen(info) + 1);
                        break;

                    case CHILD_INFO_COMMAND:
                        pipename_child = snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        char* info = child_info_command(pipename_child, child_id);
                        write(controller_pipename, info, strlen(info) + 1);
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
char* self_info_command(timer* current_timer){
    char info[100];
    char* registry = registry_info(current_timer);
    //if the string of registry info is NULL, make it contain an error message
    if(registry == NULL){
        registry = "error reading registry";
    }
    //formats the info as "State: <state> Switch: <switches> Registry: <registry info>"
    snprintf(info, sizeof(info),
        "State: %d Switch: %d Registry: %s",
        current_timer->state,
        current_timer->switches,
        registry );
    return info;
}

//gets the info of the registry of the timer and returns it as a string
char* registry_info(timer* current_timer){
    char info[100];
    char childs[100];
    childs[0] = '\0';
    int len = 0;

    //adds the child ids to the string childs, separated by commas
    for (int i = 0; i < current_timer->registry.child_num; i++) {
        if (i > 0) {
            len += snprintf(childs + len, sizeof(childs) - len, ",");
        }
        len += snprintf(childs + len, sizeof(childs) - len, "%d", current_timer->registry.child_id[i]);
    }
    
    //formats the info as "id=<id> parent_id=<parent_id> child_num=<child_num> children=[<string of children>]"
    snprintf(info, sizeof(info) - len,
        "id=%d parent_id=%d child_num=%d children=[%s]",
        current_timer->registry.id,
        current_timer->registry.parent_id,
        current_timer->registry.child_num,
        childs);

    return info;
}

//gets the info of a child of the timer and returns it as a string
char* child_info_command(char* pipename_child, char* child_id){
    //opens the pipe of the child and sends the command to get its info
    snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
    int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
    write(child_pipe, "self_info", strlen("self_info") + 1);
    close(child_pipe);
    //reads the response from the child and returns it
    char response[100];
    ssize_t bytes_read = read(child_pipe, response, sizeof(response) - 1);
    if(bytes_read >= 0){ 
        response[bytes_read] = '\0'; // Null-terminate the string
    } else {
        // handle empty response case
        response = "No response received from child.";
    }
    return response;
}