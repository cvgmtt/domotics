#include <device.h>


FILE* initDevice(int fd[], int pipe){
    close(fd[0]); 
    int msg;
    FILE* fp = fopen(".registry.txt", "a");
    if(fp == NULL || pipe < 0){
        msg = FAILURE;
        write(fd[1], &msg, sizeof(msg));
        close(fd[1]); 
        exit(FAILURE);
    }
    msg = SUCCESS;
    write(fd[1], &msg, sizeof(msg));
    close(fd[1]); 
    return fp;
}

int checkSuccess(int fd[], pid_t pid){
    close(fd[1]);
    int child_status;
        
    ssize_t bytes_read = read(fd[0], &child_status, sizeof(child_status));
    close(fd[0]); 

    if (bytes_read > 0) {
        if (child_status == FAILURE) {
            perror("could not open file \n");
            waitpid(pid, NULL, 0);
        }
        return child_status; 
    } else {
        waitpid(pid, NULL, 0);
        perror("process crashed \n");
        return FAILURE;
    }
}

int createPipe(int num, char* pipename, size_t size){
    snprintf(pipename, size, "/tmp/domotics_%d", num);
    if(mkfifo(pipename, 0644) == 0){
        printf("pipe opened correctly \n");
        return SUCCESS;
    }else{
        perror("error in opening pipe \n");
        return FAILURE;
    };
}

void kill_device(int id){
    int controller_pipe = open("/tmp/domotics_0", O_WRONLY);

    if (controller_pipe >= 0) {
        char msg[50];
        snprintf(msg, sizeof(msg), "del %d", id); 
        
        write(controller_pipe, msg, strlen(msg) + 1);
        close(controller_pipe);
        //destroy the named pipe
        char pipename[30];
        snprintf(pipename, sizeof(pipename), "/tmp/domotics_%d", id);
        unlink(pipename); 

        exit(SUCCESS);
    }
}

int confirm_del(char* pipename_parent){
    char msg[30];
    snprintf(msg, sizeof(msg), "received delete command");
    int pipe = open(pipename_parent, O_WRONLY);
    if(pipe !=-1){
        write(pipe, msg, sizeof(msg));
        close(pipe);
        return SUCCESS;
    }
    return FAILURE;
}
//gets the info of a child and returns it as a string
void child_info_command(char* pipename_child, char* pipename_parent, char* response, size_t size){
    //opens the pipe of the child and sends the command to get its info
    int child_pipe = open(pipename_child, O_RDWR);
    write(child_pipe, "self_info", strlen("self_info") + 1);
    close(child_pipe);
}

int getCommand(char* buf, char* id, char* pos, char* child_id){
    char* token = strtok(buf, " ");
    if(token != NULL){
        if(strcmp(token, "new") == 0){
            token = strtok(NULL, " ");
            if(token != NULL){
                char* id_temp = strtok(NULL, " ");
                strcpy(id, id_temp);
                if(strcmp(token, "parent") == 0){
                    printf("got in get command \n");
                    id_temp = strtok(NULL, " ");
                    //in case the interaction device already had a controller device parent
                    if(id_temp != NULL){
                        strcpy(child_id, token);
                        return CHANGE_PARENT_COMMAND;
                    }
                    return CHANGE_PARENT_COMMAND;
                } else if(strcmp(token, "child") == 0){
                    printf("child \n");
                    if(id_temp != NULL){
                        strcpy(child_id, id_temp);
                        return CHANGE_CHILD_COMMAND;
                    } else{
                        return INVALID_COMMAND;
                    }
                    
                } else {
                    return INVALID_COMMAND;
                }
            } else {
                return INVALID_COMMAND;
            }
        } else if(strcmp(token, "self_delete") == 0){
            char* id_temp = strtok(NULL, " ");
            if(id_temp == NULL){
                return INVALID_COMMAND;
            }
            return SELF_DEL_COMMAND;
        } else if(strcmp(token, "child_delete") == 0){
            char* id_temp = strtok(NULL, " ");
            if(id_temp == NULL){
                return INVALID_COMMAND;
            }
            strcpy(child_id, id_temp);
            return CHILD_DEL_COMMAND;
        } else if(strcmp(token, "switch") == 0){
            char* id_temp = strtok(NULL, " ");
            if(id_temp == NULL){
                return INVALID_COMMAND;
            }
            strcpy(id, id_temp);
            strtok(NULL, " ");  //label
            char* pos_temp = strtok(NULL, " "); 
            if(pos == NULL){
                return INVALID_COMMAND;
            }
            strcpy(pos, pos_temp);
            return SWITCH_COMMAND;
        } else if(strcmp(token, "self_info") == 0){
            char* next = strtok(NULL, " ");
            if(next != NULL){
                return INVALID_COMMAND;
            }
            return SELF_INFO_COMMAND;
        } else if(strcmp(token, "child_info") == 0){
            char* child_id_temp = strtok(NULL, " ");
            if(child_id_temp == NULL){
                return INVALID_COMMAND;
            }
            strcpy(child_id, child_id_temp);
            char* extra = strtok(NULL, " ");
            if(extra != NULL){
                return INVALID_COMMAND;
            }
            return CHILD_INFO_COMMAND;
        } else {
            return INVALID_COMMAND;
        }
    } else{
        return INVALID_COMMAND;
    }

}