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

        //pipe of the controller device to send the info of the window when requested
        char controller_pipename[20];
        snprintf(controller_pipename, sizeof(controller_pipename), "/tmp/domotics_0");


        char buf[50];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        char pipename_father[20];
        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            char info [100];

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
                    case SELF_INFO_COMMAND:
                        printf("got in window self info command \n");
                        window_info_command(&window, info, sizeof(info));
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
                    case SWITCH_COMMAND:
                        if (strcmp(pos, "open") == 0){
                            window.switches = 1;
                            window.state = 1;
                        }else if (strcmp(pos, "close") == 0){
                            window.switches = 0;
                            window.state = 0;
                        }
                        //notifies father
                        snprintf(pipename_father, sizeof(pipename_father), "/tmp/domotics_%d", window.registry.parent_id);
                        notify_parent(pipename_father, window.state);
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

void window_info_command(window* current_window, char* info, size_t size){
    snprintf(info, size,
        "State: %d Switch: %d Time: %d Parent: %d",
        current_window->state,
        current_window->switches,
        current_window->registry.time_open,
        current_window->registry.parent_id);
}

