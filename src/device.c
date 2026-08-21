#include <device.h>
#include <unistd.h>


int setup_device(int id, const char* type_name, int control_pipe[]) {
    srand(time(NULL));
    char pipename[32];
    //create and open the named pipe 
    if (createPipe(id, pipename, sizeof(pipename)) == PIPE_ERROR) {
        return -1;
    }
    int pipe_fd = open(pipename, O_RDWR);
    
    //open registry file and send success/failure message to the controller through the unnamed pipe (control pipe)
    FILE* fp = fopen(".registry.txt", "a");
    close(control_pipe[0]); 
    int msg;

    if(fp == NULL || pipe_fd < 0){
        msg = FAILURE;
        write(control_pipe[1], &msg, sizeof(msg));
        close(control_pipe[1]); 
        exit(FAILURE);
    }

    msg = SUCCESS;
    write(control_pipe[1], &msg, sizeof(msg));
    close(control_pipe[1]); 
    //write new row in registry
    fprintf(fp, "%d, %d, %s, 0, \n", id, getpid(), type_name);
    fclose(fp);
    return pipe_fd;
}

int checkSuccess(int fd[], pid_t pid){
    close(fd[1]);
    int child_status;
        
    ssize_t bytes_read = read(fd[0], &child_status, sizeof(child_status));
    close(fd[0]); 

    if (bytes_read > 0) {
        if (child_status == FAILURE) {
            printf("could not open file \n");
            waitpid(pid, NULL, 0);
        }
        return child_status; 
    } else {
        waitpid(pid, NULL, 0);
        printf("process crashed \n");
        return FAILURE;
    }
}

int createPipe(int num, char* pipename, size_t size){
    snprintf(pipename, size, "/tmp/domotics_%d", num);
    if(mkfifo(pipename, 0644) == 0){
        return SUCCESS;
    }else{
        printf("error in opening pipe \n");
        return PIPE_ERROR;
    };
}

void kill_device(int id){
    int controller_pipe = open("/tmp/domotics_0", O_WRONLY);

    if (controller_pipe >= 0) {
        char msg[MSG_SIZE];
        memset(msg, 0, sizeof(msg));
        snprintf(msg, sizeof(msg), "del %d", id); 
        
        write(controller_pipe, msg, sizeof(msg));
        close(controller_pipe);
        //destroy the named pipe
        char pipename[30];
        snprintf(pipename, sizeof(pipename), "/tmp/domotics_%d", id);
        unlink(pipename); 

        exit(SUCCESS);
    } else{
        printf("error in opening controller pipe, device not deleted \n");
    }
}

int confirm_del(char* pipename_parent){
    char msg[MSG_SIZE];
    memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg), "received delete command");
    int pipe = open(pipename_parent, O_WRONLY);
    if(pipe !=-1){
        write(pipe, msg, sizeof(msg));
        close(pipe);
        return SUCCESS;
    }
    return FAILURE;
}

void child_info_command(char* pipename_child) {
    //opens the pipe of the child and send the command to get the info
    int child_pipe = open(pipename_child, O_WRONLY); 
    if (child_pipe != -1) {
        char msg[MSG_SIZE];
        memset(msg, 0, sizeof(msg));
        strcpy(msg, "self_info");
        write(child_pipe, msg, sizeof(msg));
        close(child_pipe);
    } else {
        printf("Error: couldn't open child pipe for info request.\n");
    }
}

void send_info_to_controller(const char* info) {
    if (info == NULL || strlen(info) == 0) return;
    
    char buffer[MSG_SIZE];
    memset(buffer, 0, MSG_SIZE);
    
    strncpy(buffer, info, MSG_SIZE - 1);
    
    int controller_pipe = open("/tmp/domotics_0", O_WRONLY);
    if (controller_pipe >= 0) {
        if (write(controller_pipe, buffer, MSG_SIZE) < 0) {
            printf("write controller pipe \n");
        }
        close(controller_pipe);
    } else {
        printf("open controller pipe \n");
    }
}

void delete_interaction_device(int id, int parent_id) {
    if (parent_id != 0) {
        char pipename_parent[32];
        snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%d", parent_id);
        if (confirm_del(pipename_parent) != SUCCESS) {
            printf("error in deleting device with id %d\n", id);
        }
    }
    kill_device(id);
}
//this function waits a maximum of 8 seconds for the confirmation of the deletion of an interaction device linked to a control device
int wait_for_device_response(int my_id, char* response_buf, size_t buf_size) {
    char check_pipename[20];
    snprintf(check_pipename, sizeof(check_pipename), "/tmp/domotics_%d", my_id);
    
    int check_pipe = open(check_pipename, O_RDONLY | O_NONBLOCK);
    if (check_pipe == -1) {
        return PIPE_ERROR;
    }
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(check_pipe, &read_fds);

    struct timeval tv;
    tv.tv_sec = 8;
    tv.tv_usec = 0;

    //select make sure that the control device is not stuck while waiting
    int activity = select(check_pipe + 1, &read_fds, NULL, NULL, &tv);
    
    int result = TIME_OUT;
    if (activity > 0) {
        if (read(check_pipe, response_buf, buf_size) > 0) {
            result = SUCCESS;
        }
    }
    
    close(check_pipe);
    return result;
}

void wait_function(){
    int wait_time = 1.00 + rand() % 3;
    sleep(wait_time);
}

int send_ipc_message(const char* id, const char* message) {
    char pipename[32];
    snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", id);
    
    int fd = open(pipename, O_WRONLY | O_NONBLOCK);
    if (fd >= 0) {
        char buffer[MSG_SIZE];
        memset(buffer, 0, MSG_SIZE);
        strncpy(buffer, message, MSG_SIZE - 1);
        
        write(fd, buffer, MSG_SIZE);
        close(fd);
        return SUCCESS;
    }
    return FAILURE;
}

int getCommand(char* buf, char* id, char* pos, char* child_id){
    char* token = strtok(buf, " ");
    if(token != NULL){
        if(strcmp(token, "new_parent") == 0){
            char* id_temp = strtok(NULL, " ");
            if(id_temp != NULL){
                strcpy(id, id_temp);    
                id_temp = strtok(NULL, " ");
                //in case the interaction device already had a controller device parent
                if(id_temp != NULL){
                    strcpy(child_id, id_temp);
                }
                return CHANGE_PARENT_COMMAND;
            } else{
                printf("invalid command, new control device not specified \n");
                return INVALID_COMMAND;
            } 
        } else if(strcmp(token, "new_child") == 0){
            char* id_temp = strtok(NULL, " ");
            if(id_temp != NULL){
                strcpy(child_id, id_temp);
                return CHANGE_CHILD_COMMAND;
            } else{
                printf("invalid command, new child device not specified \n");
                return INVALID_COMMAND;
            }        
        } else if(strcmp(token, "self_delete") == 0){
            char* id_temp = strtok(NULL, " ");
            if(id_temp == NULL){
                printf("invalid command, an id is expected \n");
                return INVALID_COMMAND;
            }
            return SELF_DEL_COMMAND;
        } else if(strcmp(token, "child_delete") == 0){
            char* id_temp = strtok(NULL, " ");
            if(id_temp == NULL){
                printf("invalid command, an id is expected \n");
                return INVALID_COMMAND;
            }
            strcpy(child_id, id_temp);
            return CHILD_DEL_COMMAND;
        } else if(strcmp(token, "switch") == 0){
            //get the position
            char* pos_temp = strtok(NULL, " ");
            char* extra = strtok(NULL, " ");
            if(extra != NULL){
                printf("invalid command");
                return INVALID_COMMAND;
            }
            //return the position to switch to
            strcpy(pos, pos_temp);
            return SWITCH_COMMAND;
        } else if(strcmp(token, "switch_child") == 0){
            char* id_tmp = strtok(NULL, " ");
            char* pos_temp = strtok(NULL, " ");
            char* extra = strtok(NULL, " ");
            if(extra != NULL){
                printf("invalid command");
                return INVALID_COMMAND;
            }
            strcpy(id, id_tmp);
            strcpy(pos, pos_temp);
            return SWITCH_CHILD_COMMAND;
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
        }else if(strcmp(token, "state") == 0){
            char* pos_temp = strtok(NULL, " ");
            char* extra = strtok(NULL, " ");
            if(extra != NULL){
                printf("invalid command");
                return INVALID_COMMAND;
            }
            strcpy(pos, pos_temp);
            return STATE_CHANGE;
        } else if (strcmp(token, "set") == 0) {
            return SET_COMMAND;
        } else {
            return INVALID_COMMAND;
        }
    } else{
        return INVALID_COMMAND;
    }
}