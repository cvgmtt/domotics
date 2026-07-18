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

        //uso questo per aprire la pipe del controller e averla sempre pronta invece di doverlo aprire ogni volta,
        //forse non è la cosa migliore da fare, devo chiedere a Matteo
        char controller_pipename[20] = open("/tmp/domotics_0", O_WRONLY | O_NONBLOCK);

        while(1){
            memset(buf, 0, sizeof(buf));
            int bytes_read = read(pipe, buf, sizeof(buf));
            if(bytes_read > 0){
                command = getCommand(buf, id, pos, child_id);

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
                        char* info = self_info_command(&hub);
                        write(controller_pipename, info, strlen(info) + 1);
                        break;

                    case CHILD_INFO_COMMAND:
                        pipename_child = snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
                        char* info = child_info_command(pipename_child, child_id);
                        write(controller_pipename, info, strlen(info) + 1);
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

char* self_info_command(hub* current_hub){
    char info[100];
    char* registry = registry_info(current_hub);
    //if the string of registry info is NULL, make it contain an error message
    if(registry == NULL){
        registry = "error reading registry";
    }
    //formats the info as "State: <state> Switch: <switches> Registry: <registry info>"
    snprintf(info, sizeof(info),
        "State: %d Switch: %d Registry: %s",
        current_hub->state,
        current_hub->switches,
        registry );
    return info;
}
 
char* registry_info(hub* current_hub){
    char info[100];
    char childs[100];
    childs[0] = '\0';
    int len = 0;

    //adds the child ids to the string childs, separated by commas
    for (int i = 0; i < h->registry.child_num; i++) {
        if (i > 0) {
            len += snprintf(childs + len, sizeof(childs) - len, ",");
        }
        len += snprintf(childs + len, sizeof(childs) - len, "%d", h->registry.child_id[i]);
    }
    
    //formats the info as "id=<id parent_id=<parent_id> child_num=<child_num> children=[<string of childs>]"
    snprintf(info, sizeof(info) - len,
        "id=%d parent_id=%d child_num=%d children=[%s]",
        h->registry.id,
        h->registry.parent_id,
        h->registry.child_num,
        childs);

    return info;
}

char* child_info_command(char* pipename_child, char* child_id){
    //opens the pipe of the child and sends the command to get its info
    snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
    int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
    write(child_pipe, "self_info", strlen("self_info") + 1);
    close(child_pipe);
    //reads the response from the child and returns it
    char response[100];
    ssize_t bytes_read = read(child_pipe, response, sizeof(response) - 1);
    if(bytes_read >= 0){ 
        response[bytes_read] = '\0'; // Null-terminate the string
    } else {
        // handle empty response case
        response = "No response received from child.";
    }
    return response;
}