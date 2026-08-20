#include "fridge.h"
#include <time.h>

fridge createFridge(int _perc, int _temp, int _thermostat){
    fridge fridge;
    fridge.state = 0;
    fridge.switches = 0;
    fridge.registry.delay = 10;
    fridge.registry.time = 0;
    fridge.registry.perc = _perc;
    fridge.registry.temp = _temp;
    fridge.registry.thermostat = _thermostat;
    fridge.registry.parent_id = 0;
    return fridge;

}

int createProcessFridge(int num){
    fridge fridge = createFridge(0, 0, 0);
    int fd[2]; 
    if (pipe(fd) == -1) { 
        printf("could not open pipe"); 
        return FAILURE; 
    }
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        fridge.registry.id = num + 1;        
        int pipe = setup_device(fridge.registry.id, "Fridge", fd);
        if (pipe < 0) exit(FAILURE);

        char buf[MSG_SIZE];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        
        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            char info[MSG_SIZE];
            memset(info, 0, sizeof(info));
            char buf_copy[MSG_SIZE];
            memcpy(buf_copy, buf, MSG_SIZE);


            //update time passed in seconds since fridge was opened
            if (fridge.state == 1 && fridge.registry.time != 0) {
                time_t now = time(NULL);
                fridge.registry.time = (now - fridge.registry.time);
                if (fridge.registry.delay > 0 && fridge.registry.time >= fridge.registry.delay) {
                    //automatically close the fridge after the delay time has passed
                    fridge.state = 0;
                    fridge.switches = 0;
                    fridge.registry.time = 0;
                }
            }

            if(bytes_read > 0){
                command = getCommand(buf_copy, id, pos, child_id);
                if (command != INVALID_COMMAND) {
                    wait_function();
                }
                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        fridge.registry.parent_id = atoi(id);
                        break;
                    case SELF_DEL_COMMAND:
                        delete_interaction_device(fridge.registry.id, fridge.registry.parent_id);
                        break;
                    case SELF_INFO_COMMAND:
                        fridge_info_command(&fridge, info, sizeof(info));
                        send_info_to_controller(info);
                        break;
                    case SWITCH_COMMAND:
                        if (strcmp(pos, "on") == 0){
                            //start counting open time
                            if (fridge.state == 0) {
                                fridge.registry.time = time(NULL);
                            }
                            fridge.switches = 1;
                            fridge.state = 1;
                        } else if (strcmp(pos, "off") == 0){
                            fridge.switches = 0;
                            fridge.state = 0;
                            fridge.registry.time = 0;
                        }
                        break;

                    case SET_COMMAND:
                        memcpy(buf_copy, buf, MSG_SIZE);
                        strtok(buf_copy, " ");
                        char* attribute = strtok(NULL, " ");
                        char* value = strtok(NULL, " ");
                        if(strcmp(attribute, "delay") == 0){
                            fridge.registry.delay = (time_t) atoi(value);
                        } else if(strcmp(attribute, "fill") == 0){
                            int int_value = atoi(value);
                            if(int_value <= 100){
                                fridge.registry.perc = int_value;
                            } else {
                                printf("invalid value given, it has to be less than 100 \n");
                            }
                            
                        } else if(strcmp(attribute, "thermostat") == 0){
                            fridge.registry.thermostat = atoi(value); 
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

void fridge_info_command(fridge* current_fridge, char* info, size_t size){
    snprintf(info, size,
        "State: %d, Switch: %d,  Id: %d, Parent id=%d, Time open: %d, Delay: %d, Fill percentage: %d, Temperature: %d, Thermostat: %d",
        current_fridge->state,
        current_fridge->switches,
        current_fridge->registry.id,
        current_fridge->registry.parent_id,
        (int)current_fridge->registry.time,
        (int)current_fridge->registry.delay,
        current_fridge->registry.perc,
        current_fridge->registry.temp,
        current_fridge->registry.thermostat);
}