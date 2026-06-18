#include <device.h>


FILE* initDevice(int fd[]){
    close(fd[0]); 
    int msg;
    FILE* fp = fopen(".registry.txt", "a");
    if(fp == NULL){
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
            perror("could not open file");
            //waitpid(pid, NULL, 0);
        }
        return child_status; 
    } else {
        waitpid(pid, NULL, 0);
        perror("process crashed");
        return FAILURE;
    }
}