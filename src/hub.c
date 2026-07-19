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
        char controller_pipename[20];
        snprintf(controller_pipename, sizeof(controller_pipename), "/tmp/domotics_0");

        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            if(bytes_read > 0){
                command = getCommand(buf, id, pos, child_id);
                char info[100];
                switch(command){
                    case CHANGE_PARENT_COMMAND:
                        for(int i = 0; i < hub.registry.child_num; i++){
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
                        for(int i = 0; i < hub.registry.child_num; i++){
                            if(hub.registry.child_id[i] == -1){
                                hub.registry.child_id[i] = atoi(id);
                                hub.registry.child_switches[i] = hub.switches;
                                hub.registry.child_num++;
                                break;
                            }
                        }
                        break;
                    
                    case SELF_INFO_COMMAND:
                        hub_info_command(&hub, info, sizeof(info));
                        if(strcmp(info, "") != 0){
                            int controller_pipe = open(controller_pipename, O_WRONLY | O_NONBLOCK);
                            write(controller_pipe, info, strlen(info) + 1);
                            close(controller_pipe);
                        }
                        break;

                    case CHILD_INFO_COMMAND:
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        child_info_command(pipename_child, child_id, info, sizeof(info));
                        if(info != NULL){
                            int controller_pipe = open(controller_pipename, O_WRONLY | O_NONBLOCK);
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

void hub_info_command(hub* current_hub, char* info, size_t size){
    char registry [100];
    hub_registry_info(current_hub, registry, sizeof(registry));
    //if the string of registry info is NULL, make it contain an error message
    if(registry == NULL){
        perror("error reading registry");
        return;
    }
    //formats the info as "State: <state> Switch: <switches> Registry: <registry info>"
    snprintf(info, size,
        "State: %d Switch: %d Registry: %s",
        current_hub->state,
        current_hub->switches,
        registry );
    }
 
void hub_registry_info(hub* current_hub, char* registry, size_t size){
    char childs[100];
    childs[0] = '\0';
    int len = 0;

    //adds the child ids to the string childs, separated by commas
    for (int i = 0; i < current_hub->registry.child_num; i++) {
        if (i > 0) {
            len += snprintf(childs + len, sizeof(childs) - len, ",");
        }
        len += snprintf(childs + len, sizeof(childs) - len, "%d", current_hub->registry.child_id[i]);
    }
    
    //formats the info as "id=<id parent_id=<parent_id> child_num=<child_num> children=[<string of childs>]"
    snprintf(registry, size - len,
        "id=%d parent_id=%d child_num=%d children=[%s]",
        current_hub->registry.id,
        current_hub->registry.parent_id,
        current_hub->registry.child_num,
        childs);
}

