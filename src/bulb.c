#include <bulb.h>


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
        printf("could not open pipe 1n"); 
        return FAILURE; 
    }
    pid_t pid = fork();
    if(pid < 0){
        return FAILURE;
    };
    
    if(pid == 0){
        bulb.registry.id = num + 1;
        
        int pipe = setup_device(bulb.registry.id, "Bulb", fd);
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

            if(bytes_read > 0){
                command = getCommand(buf_copy, id, pos, child_id);
                if (command != INVALID_COMMAND) {
                    wait_function();
                }
                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        bulb.registry.parent_id = atoi(id);
                        break;
                    case SELF_DEL_COMMAND:
                        delete_interaction_device(bulb.registry.id, bulb.registry.parent_id);
                        break;
                    case SELF_INFO_COMMAND:
                        bulb_info_command(&bulb, info, sizeof(info));
                        send_info_to_controller(info);
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