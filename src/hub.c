#include "hub.h"

hub createHub(){
    hub hub;
    hub.state = 1;
    hub.switches = 1;
    hub.registry.child_num = 0;
    hub.registry.parent_id = 0;
    for(int i = 0; i < 20; i++){
        hub.registry.child_id[i] = -1;
        hub.registry.child_switches[i] = -1;
    }
    return hub;
}

int createProcessHub(int num){
    hub hub = createHub();
    int fd[2]; 
    if (pipe(fd) == -1) { 
        printf("could not open pipe \n"); return FAILURE; 
    }
    pid_t pid = fork();
    
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        hub.registry.id = num +1;
        
        int pipe = setup_device(hub.registry.id, "Hub", fd);
        if (pipe < 0) exit(FAILURE);
        char buf[MSG_SIZE];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        char pipename_child[20];
        char pipename_parent[20];

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
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] == atoi(child_id)){
                                hub.registry.child_id[i] = -1;
                                hub.registry.child_switches[i] = -1;
                                hub.registry.child_num--;
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                                int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                                if (child_pipe != -1) {
                                    write(child_pipe, buf, sizeof(buf));
                                    close(child_pipe);
                                }
                                break;
                            }
                        }
                        break;


                    case CHANGE_CHILD_COMMAND:
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] == -1){
                                hub.registry.child_id[i] = atoi(child_id);
                                hub.registry.child_switches[i] = hub.switches;
                                hub.registry.child_num++;
                                break;
                            }
                        }
                        break;
                    
                    case SELF_DEL_COMMAND:
                    //let's send the command to all interaction device linnked to the hub
                        for (int i = 0; i < 20; i++) {
                            if (hub.registry.child_id[i] != -1) {
                                int target_child = hub.registry.child_id[i];
                                
                                char pipename_child[32];
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", target_child);
                                char msg[MSG_SIZE];
                                memset(msg, 0, sizeof(msg));
                                snprintf(msg, sizeof(msg), "self_delete %d", target_child);
                                
                                int target_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                                if (target_pipe != -1) {
                                    write(target_pipe, msg, sizeof(msg));
                                    close(target_pipe);
                                } else {
                                    printf("couldn't open the pipe of device %d, assuming it's already terminated.\n", target_child);
                                }
                            }
                        }
                        
                        //let's collect the asnwers of devices
                        for (int i = 0; i < 20; i++) {
                            if (hub.registry.child_id[i] != -1) {
                                int target_child = hub.registry.child_id[i];
                                char response[MSG_SIZE];
                                
                                int result = wait_for_device_response(hub.registry.id, response, sizeof(response));
                                if (result == TIME_OUT) {
                                    printf("the child device %d didn't respond in time, assuming it's already terminated.\n", target_child);
                                } else if (result == PIPE_ERROR) {
                                    printf("error opening hub pipe for child %d.\n", target_child);
                                }
                                
                                hub.registry.child_id[i] = -1;
                                hub.registry.child_switches[i] = -1;
                                hub.registry.child_num--;
                            }
                        }
                        
                        kill_device(hub.registry.id);
                        break;

                    case CHILD_DEL_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        char msg[MSG_SIZE];
                        memset(msg, 0, sizeof(msg));
                        snprintf(msg, sizeof(msg), "self_delete %s", child_id);
                        
                        int target_pipe = open(pipename_child, O_WRONLY);
                        if(target_pipe != -1){
                            write(target_pipe, msg, sizeof(msg));
                            close(target_pipe);
                            
                            char response[MSG_SIZE];
                            int result = wait_for_device_response(hub.registry.id, response, sizeof(response));
                            if (result == SUCCESS) {
                                printf("interaction device deleted successfully\n");
                            } else if (result == TIME_OUT){
                                printf("couldn't delete child device (timeout)\n");
                                break;
                            } else if (result == PIPE_ERROR){
                                printf("error in opening hub pipe, not deleting interaction device\n");
                                break;
                            }
                        } else{
                            printf("couldn't open the pipe of the interaction device to delete it\n");
                            break;
                        }
                        
                        //here only if wait_for_device response was success
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] == atoi(child_id)){
                                hub.registry.child_id[i] = -1;
                                hub.registry.child_switches[i] = -1;
                                hub.registry.child_num--;
                                break;
                            }
                        }
                        break;
                    case SELF_INFO_COMMAND:
                        hub_info_command(&hub, info, sizeof(info));
                        send_info_to_controller(info);
                        break;

                    case CHILD_INFO_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", hub.registry.id);
                        child_info_command(pipename_child);
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

void hub_info_command(hub* current_hub, char* info, size_t size){
    char registry[512];
    hub_registry_info(current_hub, registry, sizeof(registry));
    if(registry[0] == '\0'){
        strcpy(registry, "error reading registry");
        return;
    }
    snprintf(info, size,
        "State: %d, Switch: %d, %s",
        current_hub->state,
        current_hub->switches,
        registry );
}
 
void hub_registry_info(hub* current_hub, char* registry, size_t size){
    char childs[512];
    childs[0] = '\0';
    int len = 0;

    for (int i = 0; i < 20; i++) {
        if (i > 0) {
            len += snprintf(childs + len, sizeof(childs) - len, ",");
        }
        if (current_hub->registry.child_id[i] != -1) {
            len += snprintf(childs + len, sizeof(childs) - len, "%d", current_hub->registry.child_id[i]);
        }else{
            len += snprintf(childs + len, sizeof(childs) - len, "0");
        }
    }

    snprintf(registry, size,
        "Id: %d, Parent id: %d Number of child devices: %d, Children=[%s]",
        current_hub->registry.id,
        current_hub->registry.parent_id,
        current_hub->registry.child_num,
        childs);
}
