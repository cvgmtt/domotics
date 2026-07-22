    #include "fridge.h"

    fridge createFridge(int _perc, int _temp, int _thermostat){
        fridge fridge;
        fridge.state = 0;
        fridge.switches = 1;
        fridge.registry.time_open = 0;
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
            perror("could not open pipe"); return FAILURE; 
        }
        pid_t pid = fork();
        if(pid < 0){
            return FAILURE;
        } 
        if(pid == 0){
            fridge.registry.id = num + 1;
            int success = FAILURE;        
            char pipename[20];
            do{
                success = createPipe(fridge.registry.id, pipename, sizeof(pipename));
            } while (success == FAILURE);

            int pipe = open(pipename, O_RDWR);
            FILE* fp = initDevice(fd, pipe);
            pid_t child_pid = getpid();
            int child_pid_int = (int) child_pid;

            fprintf(fp,"%d, %d, Fridge, 0, \n", fridge.registry.id, child_pid_int);
            fclose(fp);

            //pipe of the controller device to send the info of the fridge when requested
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
                char info[100];

                if(bytes_read > 0){
                    command = getCommand(buf, id, pos, child_id);

                    switch(command){
                        case CHANGE_PARENT_COMMAND:
                            fridge.registry.parent_id = atoi(id);
                            break;
                        case SELF_DEL_COMMAND:
                            if(fridge.registry.parent_id != 0){
                                char pipename_parent[20];
                                snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", fridge.registry.parent_id);
                                if(confirm_del(pipename_parent) == SUCCESS){
                                    kill_device(fridge.registry.id);
                                    break;
                                } else{
                                    printf("error in deleting device with id %d", fridge.registry.id);
                                    break;
                                }
                            }
                            kill_device(fridge.registry.id);
                            break;

                        case SELF_INFO_COMMAND:
                            printf("got in fridge self info command \n");
                            fridge_info_command(&fridge, info, sizeof(info));
                            if(strlen(info) > 0){
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
                            if (strcmp(pos, "on") == 0){
                                fridge.switches = 1;
                                fridge.state = 1;
                            }else if (strcmp(pos, "off") == 0){
                                fridge.switches = 0;
                                fridge.state = 0;
                            }
                            //qui dovrebbe poi notificare il control device che lo stato è cambiato
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
            "State: %d Switch: %d Registry: time_open=%d perc=%d temp=%d thermostat=%d id=%d parent_id=%d",
            current_fridge->state,
            current_fridge->switches,
            current_fridge->registry.time_open,
            current_fridge->registry.perc,
            current_fridge->registry.temp,
            current_fridge->registry.thermostat,
            current_fridge->registry.id,
            current_fridge->registry.parent_id);
    }