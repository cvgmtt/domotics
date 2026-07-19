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
        snprintf(controller_pipename, sizeof(controller_pipename), "/tmp/domotics_%d",bulb.registry.parent_id);

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
                printf("%s \n", buf);
                command = getCommand(buf, id, pos, child_id);
                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        printf("got in bulb change parent command \n");
                        bulb.registry.parent_id = atoi(id);
                        break;
                    case SELF_INFO_COMMAND:
                        printf("got in bulb self info command \n");
                        bulb_info_command(&bulb, info, sizeof(info));
                        if(strcmp(info, "") != 0){
                            write(controller_pipename, info, strlen(info) + 1);
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

void bulb_info_command(bulb* current_bulb, char* info, size_t size){
    snprintf(info, size,
        "State: %d Switch: %d Time: %.2f Parent: %d",
        current_bulb->state,
        current_bulb->switches,
        current_bulb->registry.time,
        current_bulb->registry.parent_id);
}