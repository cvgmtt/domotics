#include "hub.h"

hub createHub(){
    hub hub;
    hub.state = 1; //capisci se va bene
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
    int fd[2]; // fd[0] is the read end, fd[1] is the write end
    if (pipe(fd) == -1) { 
        perror("could not open pipe"); return FAILURE; 
    }
    pid_t pid = fork();
    
    if(pid < 0){
        return FAILURE;
    } 
    if(pid == 0){
        hub.registry.id = num +1;
        char pipename[20];
        int success = FAILURE;        
        do{
            success = createPipe(hub.registry.id, pipename, sizeof(pipename));
        } while (success == FAILURE);

        int pipe = open(pipename, O_RDWR);
  


        FILE* fp = initDevice(fd, pipe);
        pid_t child_pid = getpid();
        int child_pid_int = (int) child_pid;
        fprintf(fp,"%d, %d, Hub, 0, \n", hub.registry.id, child_pid_int);
        fclose(fp);
        char buf[50];
        int command;
        char id[10];
        char pos[10];
        char child_id[10];
        char pipename_child[20];
        char controller_pipename[20] = "/tmp/domotics_0";
        char pipename_parent[20];

        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            if(bytes_read > 0){
                command = getCommand(buf, id, pos, child_id);
                char info[512];
                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] == atoi(child_id)){
                                hub.registry.child_id[i] = -1;
                                hub.registry.child_switches[i] = -1;
                                hub.registry.child_num--;
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                                int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                                write(child_pipe, buf, sizeof(buf));
                                close(child_pipe);
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
                                printf("new child_num%d \n", hub.registry.child_num);
                                break;
                            }
                        }
                        break;
                    
                    case SELF_DEL_COMMAND:
                        for(int i = 0; i < 20; i++){
                            if(hub.registry.child_id[i] != -1){
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", hub.registry.child_id[i]);
                                char msg[20];
                                snprintf(msg, sizeof(msg), "self_delete %d", hub.registry.child_id[i]);
                                int out_pipe = open(pipename_child, O_WRONLY);
                                if(out_pipe != -1){
                                    write(out_pipe, msg, sizeof(msg));
                                    close(out_pipe);
                                } else{
                                    printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                                    continue;
                                }
                                
                                char check_pipename[20];
                                snprintf(check_pipename, sizeof(check_pipename), "/tmp/domotics_%d", hub.registry.id);
                                int check_pipe = open(check_pipename, O_RDONLY | O_NONBLOCK);
                                if(check_pipe != -1){
                                    fd_set read_fds;
                                    FD_ZERO(&read_fds);
                                    FD_SET(check_pipe, &read_fds);

                                    //max timeout
                                    struct timeval tv;
                                    tv.tv_sec = 1;
                                    tv.tv_usec = 0;

                                    int activity = select(check_pipe + 1, &read_fds, NULL, NULL, &tv);

                                    if(activity > 0){
                                        bytes_read = read(check_pipe, msg, sizeof(msg));
                                        if (bytes_read > 0){
                                            close(check_pipe);
                                            hub.registry.child_id[i] = -1;
                                            hub.registry.child_switches[i] = -1;
                                            hub.registry.child_num--;
                                        } else{
                                            close(check_pipe);
                                        }
                                    } else if (activity == 0){
                                        close(check_pipe);
                                        printf("couldn't delete child device, therefore not deleting control device");
                                    } else{
                                        close(check_pipe);
                                    }
                                } else{
                                    printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                                }
                            }
                        }
                        
                        //if no children kills it immediatly
                        if(hub.registry.child_num == 0){
                            kill_device(hub.registry.id);
                            break;
                        } else{
                            printf("couldn't delete all child devices, therefore not deleting control device");
                            break;
                        }

                    case CHILD_DEL_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        char msg[20];
                        snprintf(msg, sizeof(msg), "self_delete %s", child_id);
                        int out_pipe = open(pipename_child, O_WRONLY);
                        if(out_pipe != -1){
                            write(out_pipe, msg, sizeof(msg));
                            close(out_pipe);
                        } else{
                            printf("couldn't open the pipe of the interaction device to delete it");
                            break;
                        }
                        //checks
                        char check_pipename[20];
                        snprintf(check_pipename, sizeof(check_pipename), "/tmp/domotics_%d", hub.registry.id);
                        int check_pipe = open(check_pipename, O_RDONLY | O_NONBLOCK);
                        if(check_pipe != -1){
                            fd_set read_fds;
                            FD_ZERO(&read_fds);
                            FD_SET(check_pipe, &read_fds);

                            //max timeout
                            struct timeval tv;
                            tv.tv_sec = 1;
                            tv.tv_usec = 0;

                            int activity = select(check_pipe + 1, &read_fds, NULL, NULL, &tv);

                            if(activity > 0){
                                // interaction device has responded in time
                                bytes_read = read(check_pipe, msg, sizeof(msg));
                                if (bytes_read > 0){
                                    close(check_pipe);
                                    for(int i = 0; i < 20; i++){
                                        if(hub.registry.child_id[i] == atoi(child_id)){
                                            hub.registry.child_id[i] = -1;
                                            hub.registry.child_switches[i] = -1;
                                            hub.registry.child_num--;
                                            break;
                                        }
                                    }
                                }
                                printf("interaction device deleted successfully");  
                                break;
                            } else if (activity == 0){
                                close(check_pipe);
                                printf("couldn't delete child device, therefore not deleting control device");
                                break;
                            } else{
                                close(check_pipe);
                                break;
                            }
                        } else{
                            printf("couldn't open the pipe of the device to check whether interaction device was deleted");
                            break;
                        }
                    case SELF_INFO_COMMAND:
                        hub_info_command(&hub, info, sizeof(info));
                        if(strcmp(info, "") != 0 && strcmp(info, "\0") != 0){
                            int controller_pipe = open(controller_pipename, O_WRONLY);
                            if (controller_pipe < 0) {
                                perror("open controller pipe");
                                break;
                            }
                            if (write(controller_pipe, info, strlen(info) + 1) < 0) {
                                perror("write controller pipe");
                            }
                            printf("message sent back to controller\n");
                            close(controller_pipe);
                        }
                        break;

                    case CHILD_INFO_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", hub.registry.id);
                        child_info_command(pipename_child, pipename_parent, info, sizeof(info));
                        break;

                    case SWITCH_COMMAND:
                        printf("command switch reached in hub\n");
                        //changing state and switch
                        if (strcmp(pos, "on") == 0){
                            hub.switches = 1;
                            hub.state = 1;
                            printf("switched Hub %s\n", pos);
                        } else if (strcmp(pos, "off") == 0){
                            hub.switches = 0;
                            hub.state = 0;
                            printf("switched %s\n", pos);
                        }
                        //updating children 
                        for(int i = 0; i<20; i++){
                            if(hub.registry.child_switches[i] != -1){
                                snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%d", hub.registry.child_id[i] );
                                int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                                if (child_pipe >= 0){
                                    char message[50];
                                    snprintf(message, sizeof(message), "switch %s", pos);
                                    if (write(child_pipe, message, strlen(message) + 1) < 0) {
                                        perror("write child pipe");
                                    }
                                    close(child_pipe);
                                }
                            }
                        }
                        //manca l'updating dell'array child_switches
                        //lo implemento dopo quando ho un sistema di notifica da parte del figlio che funziona
                        break;
                    case SWITCH_CHILD_COMMAND:
                        printf("command switch_child reached in hub\n");
                        char message[50];
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", id );
                        int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                        if (child_pipe >= 0){
                            snprintf(message, sizeof(message), "switch %s", pos);
                            if (write(child_pipe, message, strlen(message) + 1) < 0) {
                                perror("write child pipe");
                            }
                            close(child_pipe);
                        }
                        //manca l'updating dell'array child_switches
                        //si aggiornano automaticamente gestendo il comando STATE_CHANGE (dovrebbe avere senso)
                        break;
                    case STATE_CHANGE:
                        int child_state = atoi(pos);
                        if(child_state != hub.state){
                            printf("hub's state is inconsistent");
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

void hub_info_command(hub* current_hub, char* info, size_t size){
    char registry[512];
    hub_registry_info(current_hub, registry, sizeof(registry));
    //if the string of registry info is empty, make it contain an error message
    if(registry[0] == '\0'){
        strcpy(registry, "error reading registry");
        return;
    }
    //formats the info as "State: <state> Switch: <switches> Registry: <registry info>"
    snprintf(info, size,
        "State: %d Switch: %d Registry: %s",
        current_hub->state,
        current_hub->switches,
        registry );
    //debug
    printf("hub info are created\n");
    }
 
void hub_registry_info(hub* current_hub, char* registry, size_t size){
    char childs[512];
    childs[0] = '\0';
    int len = 0;

    //iterate over the full array so the output reflects which slots are occupied
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

    
    //formats the info as "id=<id parent_id=<parent_id> child_num=<child_num> children=[<string of childs>]"
    snprintf(registry, size,
        "id=%d parent_id=%d child_num=%d children=[%s]",
        current_hub->registry.id,
        current_hub->registry.parent_id,
        current_hub->registry.child_num,
        childs);
    //debug
    printf("registry info are created\n");
}
