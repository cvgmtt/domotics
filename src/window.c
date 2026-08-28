#include "window.h"

window createWindow(){
    window window;
    window.state = 0;
    window.switches = 0;
    window.registry.time_open = 0;
    window.registry.parent_id = 0;
    return window;
}

int createProcessWindow(int num){
    window window = createWindow();
    int control_pipe[2]; 
    if (pipe(control_pipe) == -1) { 
        printf("could not open pipe"); 
        return FAILURE; 
    }
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    } 
    
    if(pid == 0){
        window.registry.id = num + 1;
        
        int pipe = setup_device(window.registry.id, "Window", control_pipe);
        if (pipe < 0) {
            exit(FAILURE); 
        }

        char buf[MSG_SIZE];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        
        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            char info [MSG_SIZE];
            memset(info, 0, sizeof(info));
            char buf_copy[MSG_SIZE];
            memcpy(buf_copy, buf, MSG_SIZE);

            if(bytes_read > 0){
                command = getCommand(buf_copy, id, pos, child_id);
                if (command != INVALID_COMMAND) {
                    wait_function();
                }
                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        window.registry.parent_id = atoi(id);
                        break;
                    case SELF_DEL_COMMAND:
                        delete_interaction_device(window.registry.id, window.registry.parent_id);
                        break;
                    case SELF_INFO_COMMAND:
                        //send info string to the controller
                        window_info_command(&window, info, sizeof(info));
                        send_ipc_message("0", info);                      
                        break;
                    case SWITCH_COMMAND:
                        if (strcmp(pos, "on") == 0){
                            window.switches = 1;
                            window.state = 1;//open 
                            window.switches = 0;
                        }else if (strcmp(pos, "off") == 0){
                            window.switches = 1;
                            window.state = 0;//closed
                            window.switches = 0;
                        }
                        break;
                    case SET_COMMAND:
                        printf("wrong device targeted, you can't set parameters of window type device \n");
                    default:
                        break;
                }
            }
        }
    } else{
        return checkSuccess(control_pipe, pid);
    }
}

void window_info_command(window* current_window, char* info, size_t size){
    snprintf(info, size,
        "State: %d, Switch: %d, Id: %d, Parent id: %d, Time: %d",
        current_window->state,
        current_window->switches,
        current_window->registry.id,
        current_window->registry.parent_id,
        current_window->registry.time_open);
}
