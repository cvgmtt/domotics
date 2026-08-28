#include "hub.h"

hub createHub(){
    hub hub;
    hub.state = 0;
    hub.switches = 0;
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
    //opening unnamed pipe for checking success in process creation
    int fd[2]; 
    if (pipe(fd) == -1) { 
        printf("could not open pipe for checking success on process creation\n"); 
        return PIPE_ERROR; 
    }
    pid_t pid = fork();
    
    if(pid < 0){
        printf("could not create process \n");
        return FAILURE;
    } 
    if(pid == 0){
        hub.registry.id = num +1;
        
        int pipe = setup_device(hub.registry.id, "Hub", fd);
        if (pipe < 0) {
            printf("could not open device named pipe\n"); 
            exit(FAILURE);
        }
        char buf[MSG_SIZE];
        char msg[MSG_SIZE];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        char pipename_child[20];
        char pipename_parent[20];
        char message[50];

        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            char info[MSG_SIZE];
            memset(info, 0, sizeof(info));
            char buf_copy[MSG_SIZE];
            memcpy(buf_copy, buf, MSG_SIZE);
            if(bytes_read > 0){
                command = getCommand(buf_copy, id, pos, child_id);
                
                //check if the state is inconsistent, if it is print warning and set the command as invalid
                for(int i = 0; i <20; i++){
                    if(check_inconsistency(hub.registry.child_switches[i], hub.switches) && hub.registry.child_switches[i] != -1){
                        command = INVALID_COMMAND;
                        printf("state inconsistency detected in hub, manual override needed \n");
                    }
                }

                if (command != INVALID_COMMAND) {
                    wait_function();
                }

                switch(command){
                    //remove from hub array the id of the interaction device and send an ipc message to it so that it unlinks from the hub
                    //this command is used when we want to link an interaction device already linked to another control device
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

                    //this command is used to add an interaction device to the hub list, meaning that it just got linked
                    case CHANGE_CHILD_COMMAND:
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] == -1){
                                hub.registry.child_id[i] = atoi(child_id);
                                hub.registry.child_switches[i] = hub.switches;
                                hub.registry.child_num++;
                                break;
                            }
                            
                        }
                        //switch the state of the interaction device to on or off based off of the hub switch
                        if(hub.switches == 0){
                            snprintf(msg, sizeof(msg), "switch off");
                        } else{
                            snprintf(msg, sizeof(msg), "switch on");
                        }
                        send_ipc_message(child_id, msg);
                        break;
                    //this command send a self delete message to all interaction device linnked to the hub and then deletes the hub itself
                    case SELF_DEL_COMMAND:
                        for (int i = 0; i < 20; i++) {
                            if (hub.registry.child_id[i] != -1) {
                                int target_child = hub.registry.child_id[i];
                                
                                char pipename_child[32];
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", target_child);
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

                    //this command deletes a specific child, but it follows a similar procedure as before
                    case CHILD_DEL_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        char msg[MSG_SIZE];
                        memset(msg, 0, sizeof(msg));
                        snprintf(msg, sizeof(msg), "self_delete %s", child_id);
                        
                        int target_pipe = open(pipename_child, O_WRONLY);
                        //if we can't open the target pipe it means the process has crashed
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
                            printf("couldn't open the pipe of the interaction device, the process might have crashed\n");
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
                        //send info string to the controller
                        hub_info_command(&hub, info, sizeof(info));
                        send_ipc_message("0", info);
                        break;

                    case CHILD_INFO_COMMAND:
                        //send info command to child
                        send_ipc_message(child_id, "self_info");
                        break;

                    case SWITCH_COMMAND:
                        //changing state and switch
                        if (strcmp(pos, "on") == 0){
                            hub.switches = 1;
                            hub.state = 1;                            
                        } else if (strcmp(pos, "off") == 0){
                            hub.switches = 0;
                            hub.state = 0;
                        }
                        //updating all children
                        for(int i = 0; i<20; i++){
                            if(hub.registry.child_switches[i] != -1){
                                snprintf(message, sizeof(message), "switch %s", pos);
                                send_ipc_message( hub.registry.child_id[i], message); 
                                
                                //update the switches array of the registry
                                if(strcmp(pos, "on") == 0){
                                    hub.registry.child_switches[i] = 1;
                                } else if(strcmp(pos, "off") == 0){
                                    hub.registry.child_switches[i] = 0;
                                }
                            }
                        }
                        break;
                    case SWITCH_CHILD_COMMAND:
                        //send the switch command to the child
                        snprintf(message, sizeof(message), "switch %s", pos);
                        send_ipc_message(id, message);                    
                                                
                        //update the switches array of the registry
                        int value = (strcmp(pos, "on") == 0) ? 1 : 0;
                        for(int i = 0; i < 20; i++) {
                            if (hub.registry.child_id[i] == atoi(id)) {
                                hub.registry.child_switches[i] = value;
                                break;
                            }
                        }
                    case SET_COMMAND:
                        printf("wrong device targeted, you can't set parameters of hub type device \n");
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
        printf("error reading registry \n");
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
        len += snprintf(childs + len, sizeof(childs) - len, "%d", current_hub->registry.child_id[i]);
    }

    snprintf(registry, size,
        "Id: %d, Parent id: %d Number of child devices: %d, Children=[%s]",
        current_hub->registry.id,
        current_hub->registry.parent_id,
        current_hub->registry.child_num,
        childs);
}
