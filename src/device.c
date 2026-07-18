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
                    token = strtok(NULL, " ");
                    //in case the interaction device already had a controller device parent
                    if(token != NULL){
                        strcpy(child_id, token);
                        return CHANGE_PARENT_COMMAND;
                    }
                    return CHANGE_PARENT_COMMAND;
                } else if(strcmp(token, "child") == 0){
                    printf("child \n");
                    return CHANGE_CHILD_COMMAND;
                } else {
                    return INVALID_COMMAND;
                }
            } else {
                return INVALID_COMMAND;
            }
        } else if(strcmp(token, "del") == 0){
            char* id_temp = strtok(NULL, " ");
            strcpy(id, id_temp);
            if(id_temp == NULL){
                return INVALID_COMMAND;
            }
            return DEL_COMMAND;
        } else if(strcmp(token, "switch") == 0){
            char* id_temp = strtok(NULL, " ");
            strcpy(id, id_temp);
            if(id_temp == NULL){
                return INVALID_COMMAND;
            }
            strtok(NULL, " ");  //label
            char* pos_temp = strtok(NULL, " "); 
            strcpy(pos, pos_temp);
            if(pos == NULL){
                return INVALID_COMMAND;
            }
            return SWITCH_COMMAND;
        }else if(strcmp(token, "self_info") == 0){
            if(strcmp(strtok(NULL, " "), "") != 0){
                return INVALID_COMMAND;
            }
            return SELF_INFO_COMMAND;
        }else if(strcmp(token, "child_info") == 0){
            //get the child id from the command 
            char* child_id_temp = strtok(NULL, " ");
            //copy the child id to the child_id variable to be used later
            strcpy(child_id, child_id_temp);
            //check if the child id is NULL, if it is, return invalid command
            if(child_id_temp == NULL){
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