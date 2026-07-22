#include "controller.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MSG_SIZE 100

int sigchld_pipe[2];

void sigchld_handler(int sig){
    char byte = 1;
    write(sigchld_pipe[1], &byte, 1);
}

int main(){
    //initialization of controller
    controller controller;
    controller.state = 1; //means on
    controller.switches = 1; //means on
    controller.registry.num = 0;
    controller.registry.id = 0;
    
    FILE* fp = fopen(".registry.txt", "w");
    if (fp == NULL) {
        printf("The file couldn't be opened.");
        return FAILURE;
    }
    fclose(fp);

    char *pipename = "/tmp/domotics_0";
    if(mkfifo(pipename, 0644) == 0){
        printf("controller pipe opened correctly \n");
    }else{
        perror("error in opening controller pipe \n");
        return FAILURE;
    };

    int controller_pipe = open(pipename, O_RDWR);
    if (controller_pipe < 0) {
        perror("error in opening controller pipe \n");
        return FAILURE;
    }

    if (pipe(sigchld_pipe) == -1) {
        perror("could not open sigchld pipe");
        return FAILURE;
    }
    fcntl(sigchld_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(sigchld_pipe[1], F_SETFL, O_NONBLOCK);

    

    int max_fd = controller_pipe;
    if (STDIN_FILENO > max_fd) max_fd = STDIN_FILENO;
    if (sigchld_pipe[0] > max_fd) max_fd = sigchld_pipe[0];
    fd_set read_fds;

    while(1){
        signal(SIGCHLD, sigchld_handler);
        handle_crashed_devices();

        printf("Enter the desired command: ");
        fflush(stdout);

        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(controller_pipe, &read_fds);
        FD_SET(sigchld_pipe[0], &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            continue; 
        }

        if (FD_ISSET(sigchld_pipe[0], &read_fds)) {
            char drain[64];
            while (read(sigchld_pipe[0], drain, sizeof(drain)) > 0);
            continue;
        }

        if (FD_ISSET(controller_pipe, &read_fds)) {
            char buffer[MSG_SIZE];
            memset(buffer, 0, sizeof(buffer));
            int bytes_read = read(controller_pipe, buffer, sizeof(buffer));
            
            if (bytes_read == sizeof(buffer)) {
                if (strncmp(buffer, "del ", 4) == 0) {
                    char* id_to_del = buffer + 4;
                    delete_device_from_registry(id_to_del);
                    printf("\nDevice %s deleted successfully.\n", id_to_del);
                } else if (strlen(buffer) > 0) {
                    printf("\n%s\n", buffer);
                }
            }
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char terminal_input[30];
            fgets(terminal_input, 30, stdin);
            terminal_input[strcspn(terminal_input, "\n")] = '\0';
            char *token = strtok(terminal_input, " ");
            pid_t controller_pid = getpid();
            char controller_pid_string[16];   
            sprintf(controller_pid_string, "%d", controller_pid);        
            if(token != NULL){
                if(strcmp(token, "list") == 0){
                    char list_info[4096] = "\0";
                    list(list_info);
                    printf("%s", list_info);
                } else if (strcmp(token, "add") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        if(strcmp(token, "hub") == 0){
                            if(createProcessHub(controller.registry.num) == FAILURE){
                                perror("failed to create hub device");
                            } else{
                                controller.registry.num++;
                            }
                            
                        } else if(strcmp(token, "timer") == 0)  {
                            if(createProcessTimer(controller.registry.num) == FAILURE){
                                perror("failed to create timer device");
                            } else{
                                controller.registry.num++;
                            }
                        } else if(strcmp(token, "bulb") == 0){
                            if(createProcessBulb(controller.registry.num) == FAILURE){
                                perror("failed to create bulb device");
                            } else{
                                controller.registry.num++;
                            }
                        } else if(strcmp(token, "window") == 0){
                            if(createProcessWindow(controller.registry.num) == FAILURE){
                                perror("failed to create window device");
                            } else{
                                controller.registry.num++;
                            }
                        } else if(strcmp(token, "fridge") == 0){
                            if(createProcessFridge(controller.registry.num) == FAILURE){
                                perror("failed to create fridge device");
                            } else{
                                controller.registry.num++;
                            }
                        }
                    } else{
                        printf("wrong input. add requires just one of these arguments: hub, timer, bulb, window, fridge\n");
                    }
         
                } else if(strcmp(token, "del") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        int result = del_command(token);
                        if (result == SUCCESS){
                            printf("initiated deletion of the device \n");
                        }else{
                            printf("couldn't find the device");
                        }
                    } else{             
                        printf("wrong input. del requires a valid id\n");
                    }
                } else if(strcmp(token, "link") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        char* child_id = token;
                        //token = strtok(NULL, " ");
                        token = strtok(NULL, " ");
                        if(token != NULL){
                            char* parent_id = token;
                            int result = link_command(child_id, parent_id);
                            if(result == SUCCESS){
                                printf("Linked %s with %s \n", child_id, parent_id);
                            }else if(result == ALREADY_LINKED){
                                printf("these two devices are already linked \n");
                            } else if(result == FAILURE){
                                perror("error in linking");
                            }
                        } else{
                        printf("wrong input. linking requires two valid ids\n");
                    }
                        
                    } else{
                        printf("wrong input. linking requires two valid ids\n");
                    }
                } else if(strcmp(token, "switch") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        char* id = token;
                        token = strtok(NULL, " ");
                        if(token != NULL){
                            char* label = token;
                            token = strtok(NULL, " ");
                            if(token != NULL){
                                char* pos = token;
                                //set the switch label of device on/off
                                //throw errors if you don't find id, don't recognise label or pos
                            } else {
                                printf("wrong input. switch requires <id> <label> <pos>\n");
                            }
                        } else{
                            printf("wrong input. switch requires <id> <label> <pos>\n");
                        }
                    } else{
                        printf("wrong input. switch requires <id> <label> <pos>\n");
                    }     
                } else if(strcmp(token, "info") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        char* id1 = token;
                        char info[MSG_SIZE];
                        memset(info, 0, sizeof(info)); 
                        get_info(id1, info);
                        if(info[0] == '\0'){
                            printf("could not get info of the device\n");
                        } else{
                            printf("Info of device with id %s:\n %s \n", id1, info);
                        }
                    } else{
                        printf("wrong input. info requires <id>\n");
                    }
                } else{
                    printf("please, provide one of these commands:\n");
                    printf("list, add <device>, del <id>, link <id1> to <id2>, switch <id> <label> <pos>, info <id>\n");                
                }
            } 
        } 
    } 
    
    close(controller_pipe);
    return 0;
} 

void list(char* list_info){
    FILE *fp = fopen(".registry.txt", "r");
    //checks if file is not empty
    if (fp != NULL) {
        //debug
        printf("registry aperto\n");
        char row[200];
        char info[MSG_SIZE];
        memset(info, 0, sizeof(info));
        //iterates through the rows of the file
        while (fgets(row, sizeof(row), fp) != NULL) {
            //debug
            printf("got a row: %s\n", row);
            
            //tokenizes the row to get the id of the device
            char* current_id = strtok(row, ", ");
            
            //calls get_info and adds the info of the device to the list_info string

            sprintf(list_info + strlen(list_info), "Device ID: %s\n", current_id);
            get_info(current_id, info);
            strcat(list_info, info);
            strcat(list_info, "\n\n");
            memset(info, 0, sizeof(info));
            }
        fclose(fp);
        return;
    } else{
        perror("error in opening registry file \n");
        return;
    }
}

int link_command(char* child_id, char* parent_id){
    char parent_to_change[5] = "0";

    int result = check_parents(parent_to_change, child_id, parent_id);

    if(strcmp(parent_to_change, parent_id) == 0){
        return ALREADY_LINKED;
    } else if (result == CONTROL_DEVICE_FULL){
        return CONTROL_DEVICE_FULL;
    } else if(result == FAILURE){
        return FAILURE;
    }
    printf("got past chek_parents");
    FILE* fp = fopen(".registry.txt", "r");
    FILE* temp = fopen("temp.txt", "w");
    char pipename_child[30];
    char pipename_parent[30];
    char pipename_old_parent[30];
    snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
    snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%s", parent_id);
    int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
    int parent_pipe = open(pipename_parent, O_WRONLY | O_NONBLOCK);
    

    char buffer[MSG_SIZE];
    if (fp == NULL || temp == NULL || child_pipe < 0 || parent_pipe < 0) {
        printf("error in linking \n");
        fclose(fp);
        fclose(temp);
        close(parent_pipe);
        close(child_pipe);
        return FAILURE;
    }

    char row[100];
    
    while (fgets(row, sizeof(row), fp) != NULL) {
        char row_copy[256];
        strcpy(row_copy, row); 
        
        char* current_id = strtok(row_copy, ", ");
        
        if (current_id != NULL) {
            char* pid_str = strtok(NULL, ", ");
            char* type_str = strtok(NULL, ", ");
            
            char* old_parent_str = strtok(NULL, ", ");
            
            char* children_str = strtok(NULL, "\n"); 
            if (children_str == NULL) {
                children_str = "";        
            }
            

            if (strcmp(current_id, child_id) == 0) {
                if(strcmp(type_str, "Hub") == 0 || strcmp(type_str, "Timer") == 0){
                    printf("first id is not an interaction device \n");
                    return FAILURE;
                }
                if(strcmp(parent_to_change, "0") == 0){
                    memset(buffer, 0, sizeof(buffer));
                    snprintf(buffer, sizeof(buffer), "new parent %s", parent_id);
                    write(child_pipe, buffer, sizeof(buffer));
                    close(child_pipe);
                } else{
                    memset(buffer, 0, sizeof(buffer));
                    snprintf(buffer, sizeof(buffer), "new parent %s %s", parent_id, child_id);
                    snprintf(pipename_old_parent, sizeof(pipename_old_parent), "/tmp/domotics_%s", parent_to_change);
                    int old_parent_pipe = open(pipename_old_parent, O_WRONLY | O_NONBLOCK);
                    if(old_parent_pipe >= 0){
                        write(old_parent_pipe, buffer, sizeof(buffer));
                        close(old_parent_pipe);
                    }

                    close(child_pipe);

                }
                

                fprintf(temp, "%s, %s, %s, %s, %s\n", current_id, pid_str, type_str, parent_id, children_str);
            } //modify old parent if there is one
            else if (strcmp(parent_to_change, "0") != 0 && strcmp(current_id, parent_to_change) == 0) {
                
                char new_children_str[200] = ""; 
                
                char* child_token = strtok(children_str, ", ");
                while (child_token != NULL) {
                    if (strcmp(child_token, child_id) != 0) {
                        strcat(new_children_str, child_token);
                        strcat(new_children_str, ", ");
                    }
                    child_token = strtok(NULL, ", ");
                }
                
                fprintf(temp, "%s, %s, %s, %s, %s\n", current_id, pid_str, type_str, old_parent_str, new_children_str);
            } 

            //modify new parent
            else if (strcmp(current_id, parent_id) == 0) {
                if(strcmp(type_str, "Hub") == 0 || strcmp(type_str, "Timer") == 0){
                    memset(buffer, 0, sizeof(buffer));
                    snprintf(buffer, sizeof(buffer), "new child %s", child_id);
                    write(parent_pipe, buffer, sizeof(buffer));
                    close(parent_pipe);

                    if(strcmp(type_str, "Hub") == 0){
                        row[strcspn(row, "\n")] = '\0';
                        fprintf(temp, "%s%s, \n", row, child_id);
                    } else {
                        fprintf(temp, "%s, %s, %s, 0, %s, \n", current_id, pid_str, type_str, child_id);
                    }
                
                } else{
                    perror("second id is not a control device \n");
                    return FAILURE;
                } 
                
            } 
            
            
            else {
                fprintf(temp, "%s", row); 
            }
        }
    }

    fclose(fp);
    fclose(temp);
    remove(".registry.txt");
    rename("temp.txt", ".registry.txt");

    return SUCCESS;
}

int check_parents(char* parent_to_change, char* child_id, char* parent_id){
    int result = CONTROL_DEVICE_INCOMPLETE;
    FILE* fp_scan = fopen(".registry.txt", "r");
    if (fp_scan != NULL) {
        char scan_row[200];
        while (fgets(scan_row, sizeof(scan_row), fp_scan) != NULL) {
            char scan_copy[200];
            strcpy(scan_copy, scan_row);
            char* curr = strtok(scan_copy, ", ");
            strtok(NULL, ", "); 
            char* type_str = strtok(NULL, ", ");
            char* old_p = strtok(NULL, ", "); 
            if (curr != NULL && strcmp(curr, child_id) == 0) {
                if (old_p != NULL) {
                    strcpy(parent_to_change, old_p);
                }
            } else if (strcmp(curr, parent_id) == 0) {
                char* children_str = strtok(NULL, "\n"); // lista figli
                int child_count = 0;
                if (children_str != NULL) {
                    char* temp_child = strtok(children_str, ", ");
                    while (temp_child != NULL) {
                        child_count++;
                        temp_child = strtok(NULL, ", ");
                    }
                }
                if (type_str != NULL && strcmp(type_str, "Hub") == 0 && child_count >= 20) {
                    result = CONTROL_DEVICE_FULL;
                } 
                else if (type_str != NULL && strcmp(type_str, "Timer") == 0 && child_count >= 1) {
                    result = CONTROL_DEVICE_FULL;
                }
            }
        }
    fclose(fp_scan);
    return result;
    }
    return FAILURE;
}

//function to get the infos of a device given its id
//if it doesn't exist or is an interaction device without a parent returns NULL
void get_info(char* id, char* info) {
    //debug
    printf("got into get_info\n");
    //call get_device_row to find the row of registry.txt corresponding to the device with the given id
    char row[200];
    get_device_row(id, row);
    if (strcmp(row, "\0") != 0) {
        printf("is row empty? %s\n", strcmp(row, "\0") == 0 ? "true" : "false");
        //debug
        printf("got the row and is not null\n");
        //copy the row to avoid modifying the original string
        char row_copy[30];
        strcpy(row_copy, row);

        strtok(row_copy, ", "); // id
        strtok(NULL, ", "); // pid
        char* type = strtok(NULL, ", "); // type
        char pipename[30];
        char controller_pipename[30];
        snprintf(controller_pipename, sizeof(controller_pipename), "/tmp/domotics_0");

        //if the device is a control device, send a message that returns its info, 
        //if not find the parent of the interaction device and send it a message requesting info of the child 
        if (strcmp(type, "Hub") == 0 || strcmp(type, "Timer") == 0){
            //open pipe and send a message to the control device to get its info
            snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", id);
            int self_pipe = open(pipename, O_WRONLY);
            if (self_pipe < 0) {
                perror("open self pipe");
                return;
            }
            char msg[MSG_SIZE];
            memset(msg, 0, sizeof(msg));
            strcpy(msg, "self_info");
            if (write(self_pipe, msg, sizeof(msg)) < 0) {
                perror("error writing to self pipe");
                close(self_pipe);
                return;
            }
            printf("message sent\n");
            close(self_pipe);
        }else{
            //get the parent of the interaction device
            char* parent = strtok(NULL, ", "); // parent
            //handle case where parent is 0, which means is an interaction device without a parent
            if(strcmp(parent, "0") == 0){
                printf("Device with id %s is an interaction device without a parent\n", id);
                //send message directly to the device so it reports its own info
                snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", id);
                int child_pipe = open(pipename, O_WRONLY);
                if (child_pipe < 0) {
                    perror("open child pipe");
                    return;
                }
                char msg[MSG_SIZE];
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "self_info");
                if (write(child_pipe, msg, sizeof(msg)) < 0) {
                    perror("error writing to child pipe");
                    close(child_pipe);
                    return;
                }
                printf("message sent\n");
                close(child_pipe);
            }else{
                //send message to the parent to get the info of the child
                snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", parent);
                int parent_pipe = open(pipename, O_WRONLY);
                if (parent_pipe < 0) {
                    perror("error opening parent pipe");
                    return;
                }
                char message[MSG_SIZE];
                memset(message, 0, sizeof(message));
                snprintf(message, sizeof(message), "child_info %s", id);
                if (write(parent_pipe, message, sizeof(message)) < 0) {
                    perror("error writing to parent pipe");
                    close(parent_pipe);
                    return;
                }
                printf("message sent\n");
                close(parent_pipe);
            } 
        }

        char response[MSG_SIZE];
        memset(response, 0, sizeof(response)); 
        int controller_pipe = open(controller_pipename, O_RDONLY);
        if (controller_pipe < 0) {
            perror("error opening controller pipe");
            return;
        }
        ssize_t bytes_read = read(controller_pipe, response, sizeof(response));
        printf("reading response\n");
        if (bytes_read == sizeof(response)) {
            strcpy(info, response);
        } else {
            strcpy(info, "\0");
            perror("No response received from device \n");
        }
        close(controller_pipe); 
        return; 
    } else {
        //id does not exist in the registry, return empty string
        strcpy(info, "\0");
        printf("Device with id %s does not exist in the registry.\n", id);
        
        return;
    }
}

//searches the registry for the row corresponding to the device with the given id and returns it, if it doesn't exist returns NULL
void get_device_row(char* id, char* row_copy) {
    //debug
    printf("got into get_device_row\n");

    FILE *fp = fopen(".registry.txt", "r");
    printf("opened registry file \n");

    strcpy(row_copy, "\0"); 
    if (fp != NULL) {
        printf("registry aperto\n");
        char row[200];
        //iterates through the rows of the file
        while (fgets(row, sizeof(row), fp) != NULL) {
            //debug
            printf("got a row: %s\n", row);
            //copies the row to avoid modifying the original string
            char current_row[200];
            strcpy(current_row, row);
            
            //tokenizes the row to get the id of the device
            char* current_id = strtok(current_row, ", ");
            //checks if the id of the device is the same as the one passed as argument
            if (current_id != NULL && strcmp(current_id, id) == 0) {
                printf("device found with id %s \n", id);
                //copy the row to the output parameter and return
                strcpy(row_copy, row);
                fclose(fp);
                printf("closed registry file \n");
                return;
            }
        }
        fclose(fp);
        printf("id not found\n");
        return; 
    } else{
        perror("error in opening registry file \n");
        return;
    }
}

void delete_device_from_registry(char* id_to_delete) {
    FILE* fp = fopen(".registry.txt", "r");
    FILE* temp = fopen("temp.txt", "w");
    if (fp == NULL || temp == NULL) {
        if (fp) fclose(fp);
        if (temp) fclose(temp);
        return;
    }

    char row[256];
    while (fgets(row, sizeof(row), fp) != NULL) {
        char row_copy[256];
        strcpy(row_copy, row); 
        
        char* current_id = strtok(row_copy, ", ");
        if (current_id != NULL) {
            if (strcmp(current_id, id_to_delete) == 0) {
                continue; 
            }
            
            char* pid_str = strtok(NULL, ", ");
            char* type_str = strtok(NULL, ", ");
            char* parent_str = strtok(NULL, ", ");
            char* children_str = strtok(NULL, "\n"); 
            
            if (children_str == NULL) {
                children_str = "";        
            }

            if (strcmp(type_str, "Hub") == 0 || strcmp(type_str, "Timer") == 0) {
                char new_children_str[200] = ""; 
                char* child_token = strtok(children_str, ", ");
                while (child_token != NULL) {
                    if (strcmp(child_token, id_to_delete) != 0) {
                        strcat(new_children_str, child_token);
                        strcat(new_children_str, ", ");
                    }
                    child_token = strtok(NULL, ", ");
                }
                fprintf(temp, "%s, %s, %s, %s, %s\n", current_id, pid_str, type_str, parent_str, new_children_str);
            } else {
                fprintf(temp, "%s", row); 
            }
        }
    }

    fclose(fp);
    fclose(temp);
    remove(".registry.txt");
    rename("temp.txt", ".registry.txt");
}

int del_command(char* id){
    char row[256];
    get_device_row(id, row);
    if(strlen(row) > 0){
        char row_copy[256];
        strcpy(row_copy, row);
        strtok(row_copy, ", "); 
        strtok(NULL, ", "); 
        char pipename[20];
        char* type = strtok(NULL, ", "); 
        char* parent_id = strtok(NULL, ", ");
        char msg[MSG_SIZE];
        memset(msg, 0, sizeof(msg));
        if(strcmp(type, "Hub") == 0 || strcmp(type, "Timer") == 0 || strcmp(parent_id, "0") == 0){
            snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", id);
            snprintf(msg, sizeof(msg), "self_delete %s", id);
        } else if(strcmp(parent_id, "0") != 0){
                snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", parent_id);
                snprintf(msg, sizeof(msg), "child_delete %s", id);
        }
        int pipe = open(pipename, O_WRONLY);
        if(pipe > 0){
            write(pipe, msg, sizeof(msg));
            close(pipe);
            return SUCCESS;
        } 
        printf("couldn't send the delete command");
        return FAILURE;

    }else{
        return FAILURE;
    }
}

void handle_crashed_devices() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        char id[10];
        memset(id, 0, sizeof(id));
        char row[256];
        memset(row, 0, sizeof(row));
        
        FILE* fp = fopen(".registry.txt", "r");
        if (fp) {
            char temp_row[256];
            while (fgets(temp_row, sizeof(temp_row), fp) != NULL) {
                char row_copy[256];
                strcpy(row_copy, temp_row);
                char* curr_id = strtok(row_copy, ", ");
                char* curr_pid = strtok(NULL, ", ");
                if (curr_pid != NULL && atoi(curr_pid) == pid) {
                    strcpy(id, curr_id);
                    strcpy(row, temp_row);
                    break;
                }
            }
            fclose(fp);
        }

        if (strlen(id) > 0) {
            printf("\ndevice %s crashed unexpectedly.\n", id);

            char row_copy[256];
            strcpy(row_copy, row);
            strtok(row_copy, ", "); 
            strtok(NULL, ", "); 
            char* type = strtok(NULL, ", ");
            char* parent_id_str = strtok(NULL, ", ");
            char* children_str = strtok(NULL, "\n");

            if (children_str == NULL) {
                children_str = "";
            }

            if (strcmp(type, "Hub") == 0 || strcmp(type, "Timer") == 0) {
                if (strlen(children_str) > 0) {
                    char children_ids[20][10];
                    int children_count = 0;

                    char* child_token = strtok(children_str, ", ");
                    while (child_token != NULL && children_count < 20) {
                        char clean_child[10];
                        memset(clean_child, 0, sizeof(clean_child));
                        if (sscanf(child_token, "%s", clean_child) == 1 && strlen(clean_child) > 0) {
                            strcpy(children_ids[children_count], clean_child);
                            children_count++;
                        }
                        child_token = strtok(NULL, ", ");
                    }

                    for (int i = 0; i < children_count; i++) {
                        char pipename_child[30];
                        snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", children_ids[i]);
                        int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
                        if (child_pipe >= 0) {
                            char msg[MSG_SIZE];
                            memset(msg, 0, sizeof(msg));
                            snprintf(msg, sizeof(msg), "new parent 0");
                            write(child_pipe, msg, sizeof(msg));
                            close(child_pipe);
                        }
                        link_command(children_ids[i], "0");
                    }
                }
            }
            
            delete_device_from_registry(id);
            
            char pipename[30];
            snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", id);
            unlink(pipename);
        }
    }
}
