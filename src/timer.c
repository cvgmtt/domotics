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
        char controller_pipename[20];
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
                        int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                        write(child_pipe, buf, sizeof(buf));
                        close(child_pipe);
                        break;

                    case CHANGE_CHILD_COMMAND:
                        timer.registry.child_id = atoi(id);
                        break;

                    case SELF_INFO_COMMAND:
                        timer_info_command(&timer, info, sizeof(info));
                        if(strcmp(info, "") != 0){
                            int controller_pipe = open(controller_pipename, O_WRONLY | O_NONBLOCK);
                            write(controller_pipe, info, strlen(info) + 1);
                            close(controller_pipe);
                        }                        
                        break;

                    case CHILD_INFO_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        child_info_command(pipename_child, child_id, info, sizeof(info));
                        if(info != NULL){
                            int controller_pipe = open(controller_pipename, O_WRONLY | O_NONBLOCK);
                            write(controller_pipe, info, strlen(info) + 1);
                            close(controller_pipe);
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
    if(registry == NULL){
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

