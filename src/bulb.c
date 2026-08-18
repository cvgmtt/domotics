#include "../include/bulb.h"

bulb createBulb(){
    bulb bulb;
    bulb.state = 0;
    bulb.switches = 1;
    bulb.registry.time = 0.0;
    bulb.registry.parent_id = 0;
    return bulb;
}

int createProcessBulb(int num){
    bulb bulb = createBulb();
    int fd[2]; 
    if (pipe(fd) == -1) { 
        perror("could not open pipe"); return FAILURE; 
    }
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    };
    
    if(pid == 0){
        bulb.registry.id = num + 1;
        int success = FAILURE;
        char pipename[20];
        do{
            success = createPipe(bulb.registry.id, pipename, sizeof(pipename));
        } while (success == FAILURE);

        //with O_RDONLY it blocks the cpu if you have many processes
        int pipe = open(pipename, O_RDWR);
        
        FILE* fp = initDevice(fd, pipe);
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Bulb, 0, \n", bulb.registry.id, child_pid_int);
        fclose(fp);

        //pipe of the controller device to send the info of the bulb when requested
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
            char info[100];

            if(bytes_read > 0){
                printf("%s \n", buf);
                command = getCommand(buf, id, pos, child_id);
                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        printf("got in bulb change parent command \n");
                        bulb.registry.parent_id = atoi(id);
                        break;
                    case SELF_DEL_COMMAND:
                        if(bulb.registry.parent_id != 0){
                            char pipename_parent[20];
                            snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", bulb.registry.parent_id);
                            if(confirm_del(pipename_parent) == SUCCESS){
                                kill_device(bulb.registry.id);
                                break;
                            } else{
                                printf("error in deleting device with id %d", bulb.registry.id);
                                break;
                            }
                        }
                        kill_device(bulb.registry.id);
                        break;

                    case SELF_INFO_COMMAND:
                        printf("got in bulb self info command \n");
                        bulb_info_command(&bulb, info, sizeof(info));
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
                        if (strcmp(pos, "on") == 0){
                            bulb.switches = 1;
                            bulb.state = 1;  
                        }else if (strcmp(pos, "off") == 0){
                            bulb.switches = 0;
                            bulb.state = 0;
                        }
                        //notifies father
                        snprintf(pipename_father, sizeof(pipename_father), "/tmp/domotics_%d", bulb.registry.parent_id);
                        notify_parent(pipename_father, bulb.state);
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

void bulb_info_command(bulb* current_bulb, char* info, size_t size){
    snprintf(info, size,
        "State: %d Switch: %d Time: %d Parent: %d",
        current_bulb->state,
        current_bulb->switches,
        current_bulb->registry.time,
        current_bulb->registry.parent_id);
}