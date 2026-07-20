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
                    case SELF_INFO_COMMAND:
                        printf("got in window self info command \n");
                        window_info_command(&window, info, sizeof(info));
                        if(strcmp(info, "") != 0){
                            int controller_pipe = open(controller_pipename, O_RDWR);
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

void window_info_command(window* current_window, char* info, size_t size){
    snprintf(info, size,
        "State: %d Switch: %d Time: %d Parent: %d",
        current_window->state,
        current_window->switches,
        current_window->registry.time_open,
        current_window->registry.parent_id);
}