#include "controller.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

int sigchld_pipe[2];

void sigchld_handler(int sig){
    char byte = 1;
    if(write(sigchld_pipe[1], &byte, 1) < 0) {
        printf("error in writing to sigchld_pipe, a process has crashed");
    } 
}



int main(){
    //initialization of controller
    controller controller;
    controller.state = 1; //means on
    controller.switches = 1; //means on
    controller.registry.num = 0;
    controller.registry.id = 0;
    
    //create and initialize registry
    FILE* fp = fopen(".registry.txt", "w");
    if (fp == NULL) {
        printf("The file couldn't be opened.\n");
        return FAILURE;
    }
    fclose(fp);

    //create controller pipe
    char *pipename = "/tmp/domotics_0";
    if(mkfifo(pipename, 0644) == 0){
    }else{
        perror("error in opening controller pipe\n");
        return PIPE_ERROR;
    }

    int controller_pipe = open(pipename, O_RDWR);
    if (controller_pipe < 0) {
        perror("error in opening controller pipe\n");
        return PIPE_ERROR;
    }

    //creating sigchld pipe for checking crashed processes. setting to nonblock so that if multiple processes crash at the same time the program don't get stuck
    if (pipe(sigchld_pipe) == -1) {
        perror("could not open sigchld pipe\n");
        return PIPE_ERROR;
    }
    fcntl(sigchld_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(sigchld_pipe[1], F_SETFL, O_NONBLOCK);

    //in order to determine the higher limit of the descriptors to check
    int max_fd = controller_pipe;
    if (STDIN_FILENO > max_fd) {
        max_fd = STDIN_FILENO;
    }
    if (sigchld_pipe[0] > max_fd){
        max_fd = sigchld_pipe[0];
    } 
    fd_set read_fds;

    while(1){
        //checks whether a child process has crashed
        signal(SIGCHLD, sigchld_handler);
        handle_crashed_devices();

        printf("Enter the desired command: ");
        fflush(stdout);

        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(controller_pipe, &read_fds);
        FD_SET(sigchld_pipe[0], &read_fds);

        //select pause the execution of the program till one of the pipe monitored receive something. no clock cycles consumption
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            continue; 
        }

        //clean sigchld pipe

        if (FD_ISSET(sigchld_pipe[0], &read_fds)) {
            char temp[10];
            while (read(sigchld_pipe[0], temp, sizeof(temp)) > 0);
        }

        //handle messages sent to controller
        if (FD_ISSET(controller_pipe, &read_fds)) {
            char buffer[MSG_SIZE];
            memset(buffer, 0, sizeof(buffer));
            int bytes_read = read(controller_pipe, buffer, sizeof(buffer));
            
            if (bytes_read > 0) {
                if (strncmp(buffer, "del ", 4) == 0) {
                    char* id_to_del = buffer + 4;
                    delete_device_from_registry(id_to_del);
                    printf("\nDevice %s deleted successfully.\n", id_to_del);
                } else if (strlen(buffer) > 0) {
                    printf("\n%s\n", buffer);
                }
            }
        }

        //terminal input
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char terminal_input[MSG_SIZE];
            if (fgets(terminal_input, sizeof(terminal_input), stdin) == NULL) {
            }
            terminal_input[strcspn(terminal_input, "\n")] = '\0';
            
            char input_copy[MSG_SIZE];
            strncpy(input_copy, terminal_input, MSG_SIZE);
            char *token = strtok(input_copy, " ");
            
            if(token != NULL){
                if(strcmp(token, "list") == 0){
                    char list_info[4096] = "\0";
                    list(list_info);
                    printf("\n%s", list_info);
                } else if (strcmp(token, "add") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        int creation_success = FAILURE;
                        if (strcmp(token, "hub") == 0){
                            creation_success = createProcessHub(controller.registry.num);
                        } else if (strcmp(token, "timer") == 0){
                            creation_success = createProcessTimer(controller.registry.num);   
                        } else if (strcmp(token, "bulb") == 0){
                            creation_success = createProcessBulb(controller.registry.num);
                        } else if (strcmp(token, "window") == 0) {
                            creation_success = createProcessWindow(controller.registry.num);
                        } else if (strcmp(token, "fridge") == 0) {
                            creation_success = createProcessFridge(controller.registry.num);
                        } else {
                            printf("wrong input. add requires just one of these arguments: hub, timer, bulb, window, fridge\n");
                            continue;
                        }

                        if (creation_success == FAILURE || creation_success == PIPE_ERROR) {
                            printf("failed to create %s device\n", token);
                        } else {
                            printf("device created correctly\n");
                            controller.registry.num++;
                        }
                    } else{
                        printf("wrong input. add requires just one of these arguments: hub, timer, bulb, window, fridge\n");
                    }
                } else if(strcmp(token, "del") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        int result = del_command(token);
                        if (result == SUCCESS){
                            printf("initiated deletion of the device\n");
                        }else{
                            printf("couldn't find the device\n");
                        }
                    } else{             
                        printf("wrong input. del requires a valid id\n");
                    }
                } else if(strcmp(token, "link") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        char* child_id = token;
                        token = strtok(NULL, " ");
                        if (token != NULL && strcmp(token, "to") == 0) {
                            token = strtok(NULL, " ");
                        }
                        
                        if(token != NULL){
                            char* parent_id = token;
                            int result = link_command(child_id, parent_id);
                            if(result == SUCCESS){
                                printf("Linked %s with %s\n", child_id, parent_id);
                            }else if(result == ALREADY_LINKED){
                                printf("these two devices are already linked\n");
                            } else if(result == CONTROL_DEVICE_FULL){
                                printf("control device has reached its maximum capacity\n");
                            } else if(result == FAILURE){
                                printf("error in linking\n");
                            } else if(result == INVALID_COMMAND){
                                printf("invalid command\n");
                            }
                        } else{
                            printf("wrong input. linking requires two valid ids\n");
                        }
                    } else{
                        printf("wrong input. linking requires two valid ids\n");
                    }
                } else if(strcmp(token, "switch") == 0){
                    char* id = strtok(NULL, " ");
                    
                    if(id != NULL){
                        char* label = strtok(NULL, " ");
                        int result;
                        char* pos = strtok(NULL, ", ");

                        if(strcmp(pos, "on") != 0 && strcmp(pos, "off") != 0){
                            printf("wrong input. switch requires a valid position such as: on, off\n");
                        }else if(strcmp(label, "power") != 0 && strcmp(label, "close")!= 0 && strcmp(label, "open") != 0){
                            printf("wrong input. switch requires a valid label such as: power, close or open\n");
                        }else if(strtok(NULL, " ") == NULL){
                                result = switch_device(id, label, pos);
                                if(result == FAILURE){
                                    printf("error occurred while switching the device\n");
                                }else{
                                    printf("switch command sent to device %s\n", id);
                                }
                        } else {
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

int switch_device(char* id, char* label, char* pos){
    //get the row of the device
    char row[200];
    get_device_row(id, row);

    //check for valid row
    if(strcmp(row, "\0") == 0){
        return FAILURE; //row empty id not found
    }

    //copy row to extract parametres we need
    char row_copy[200];
    strcpy(row_copy, row);

    strtok(row_copy, ", "); // id
    strtok(NULL, ", "); // pid
    char* type = strtok(NULL, ", ");//type
    char* parent_id = strtok(NULL, ", "); //parent_id

    char message[50];

    //check if label is consisent with type of device
    if(strcmp(label, "power") == 0){
        //check if is control device or a bulb
        if(strcmp(type, "Timer") == 0 || strcmp(type, "Hub") == 0){
            //send the switch command to the control device 
            snprintf(message, sizeof(message), "switch %s", pos);
            send_ipc_message(id, message);
        } else if(strcmp(type, "Bulb") == 0){
            if(strcmp(parent_id, "0") == 0){
                //send message to bulb device if it doesn't have a parent
                snprintf(message, sizeof(message), "switch %s", pos);
                send_ipc_message(id, message);
            } else if(parent_id != NULL){
                printf("couldn't perform action, device %s is linked to a control device\n", id);
                return FAILURE;
            }else{
                printf("couldn't perform action for id %s, parent not specified\n", id);
                return FAILURE;
            }
        }else{
            printf("inconsistent label and type of device, cannot perform action\n");
            return FAILURE;
        }
    }else if ((strcmp(label, "open") == 0 || strcmp(label, "close") == 0) && (strcmp(type, "Fridge") == 0 || strcmp(type, "Window") == 0)){
        //switch of a fridge or window device

        //if the command is "close off" or "open on" send pos = on, otherwise set pos = off
        char command[100];
        char actual_pos[10];
        
        snprintf(command, sizeof(command), "%s %s", label, pos);
        
        if(strcmp(command, "close off") == 0 || strcmp(command, "open on") == 0 ){
            strcpy(actual_pos, "on");
        } else{
            strcpy(actual_pos, "off");
        }

        if(strcmp(parent_id, "0") == 0){
            //send message to device itself if it doesn't have a parent
            snprintf(message, sizeof(message), "switch %s %s", id, actual_pos);
            send_ipc_message(id, message);
        }else if(parent_id != NULL){
            printf("couldn't perform action, device %s is linked to a control device\n", id);
            return FAILURE;
        }else{
            printf("couldn't perform action for id %s, parent not specified\n", id);
            return FAILURE;
        }
    } else{
        printf("inconsistent label and type of device, cannot perfrom action\n");
        return FAILURE;
    }
    return SUCCESS;
};

//lists all info of the devices in the registry.
void list(char* list_info){
    int count = 0;
    FILE *fp = fopen(".registry.txt", "r");
    if (fp != NULL) {
        char row[200];
        char dummy_info[MSG_SIZE]; 
        
        printf("retrieving devices information...\n");
        printf("Enter the desired command: ");
        fflush(stdout);
        while (fgets(row, sizeof(row), fp) != NULL) {
            char row_copy[200];
            strcpy(row_copy, row);
            char* current_id = strtok(row_copy, ", ");
            if (current_id != NULL) {
                count++;
                get_info(current_id, dummy_info); 
            }
        }
        fclose(fp);

        if(count == 0){
            strcat(list_info, "no devices active at the moment, add one using the add command \n");
            return;
        }

        int controller_pipe = open("/tmp/domotics_0", O_RDONLY | O_NONBLOCK);
        if (controller_pipe < 0) {
            perror("error in opening controller pipe for list\n");
            return;
        }

        int received = 0;
        fd_set read_fds;
        struct timeval tv;

        while (received < count) {
            FD_ZERO(&read_fds);
            FD_SET(controller_pipe, &read_fds);
            
            tv.tv_sec = 8; 
            tv.tv_usec = 0;
            
            int activity = select(controller_pipe + 1, &read_fds, NULL, NULL, &tv);
            
            if (activity > 0 && FD_ISSET(controller_pipe, &read_fds)) {
                char response[MSG_SIZE];
                memset(response, 0, sizeof(response));
                
                if (read(controller_pipe, response, sizeof(response)) > 0) {
                    if (strncmp(response, "del ", 4) == 0) {
                        char* id_to_del = response + 4;
                        delete_device_from_registry(id_to_del);
                    } else {
                        strcat(list_info, response);
                        strcat(list_info, "\n\n");
                        received++;
                    }
                }
            } else {
                strcat(list_info, "some devices did not respond in time.\n\n");
                break;
            }
        }
        close(controller_pipe);
    } else {
        perror("error in opening registry file\n");
    }
}

//this function updates the registry.txt file and sends ipc messages to interaction and control devices.
int link_command(char* child_id, char* parent_id){
    char parent_to_change[10] = "0"; 
    if(strcmp(child_id, parent_id) == 0){
        printf("first and second id provided are the same.\n");
        return INVALID_COMMAND;
    }

    int result = check_parents(parent_to_change, child_id, parent_id);

    if(strcmp(parent_to_change, parent_id) == 0){
        return ALREADY_LINKED;
    } else if (result == CONTROL_DEVICE_FULL){
        return CONTROL_DEVICE_FULL;
    } else if (result == INVALID_COMMAND){
        return INVALID_COMMAND;
    } else if(result == FAILURE){
        return FAILURE;
    }

    FILE* fp = fopen(".registry.txt", "r");
    FILE* temp = fopen("temp.txt", "w");
    
    if (fp == NULL || temp == NULL) {
        if (fp) fclose(fp);
        if (temp) fclose(temp);
        return FAILURE;
    }

    char row[200];
    char msg[MSG_SIZE];
    
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
            
            //first case: the device is the interaction device
            if (strcmp(current_id, child_id) == 0) {
                //if it was not linked to anything, just send the command new parent to the interaction device
                if(strcmp(parent_to_change, "0") == 0){
                    snprintf(msg, sizeof(msg), "new_parent %s", parent_id);
                    send_ipc_message(child_id, msg);
                } else{
                    //if it was linked to another control device, other than sending the command to the interaction device
                    //send an ipc command to the old control device to unlink it
                    snprintf(msg, sizeof(msg), "new_parent %s %s", parent_id, child_id);
                    send_ipc_message(parent_to_change, msg);
                    
                    snprintf(msg, sizeof(msg), "new_parent %s", parent_id);
                    send_ipc_message(child_id, msg);
                }

                fprintf(temp, "%s, %s, %s, %s, %s\n", current_id, pid_str, type_str, parent_id, children_str);
            } 
            //second case: the device is the old control device to forget (if there is one). 
            //we need to remove from the registry the interaction device id from the list of children of the old control device 
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
            //third case: the device is the new control device
            else if (strcmp(current_id, parent_id) == 0) {
                snprintf(msg, sizeof(msg), "new_child %s", child_id);
                send_ipc_message(parent_id, msg);

                if(strcmp(type_str, "Hub") == 0){
                    row[strcspn(row, "\n")] = '\0';
                    fprintf(temp, "%s%s, \n", row, child_id);
                } else {
                    fprintf(temp, "%s, %s, %s, 0, %s, \n", current_id, pid_str, type_str, child_id);
                }
            } else {
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

//this function checks whether the devices are respectivly interaction device and control device, and if the control device has still space for children devices.
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
                if(strcmp(type_str, "Hub") == 0 || strcmp(type_str, "Timer") == 0){
                    printf("first id is not an interaction device\n");
                    fclose(fp_scan);
                    return INVALID_COMMAND;
                }
                if (old_p != NULL) {
                    strcpy(parent_to_change, old_p);
                }
            } else if (curr != NULL && strcmp(curr, parent_id) == 0) {
                if(strcmp(type_str, "Bulb") == 0 || strcmp(type_str, "Window") == 0 || strcmp(type_str, "Fridge") == 0){
                    printf("second id is not a control device\n");
                    fclose(fp_scan);
                    return INVALID_COMMAND;
                }
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
//function to get the infos of a device given its id
//if it doesn't exist or is an interaction device without a parent returns NULL
void get_info(char* id, char* info) {
    char row[200];
    get_device_row(id, row);
    
    if (strcmp(row, "\0") != 0) {
        char row_copy[200];
        strcpy(row_copy, row);

        strtok(row_copy, ", "); // id
        strtok(NULL, ", "); // pid
        char* type = strtok(NULL, ", "); // type
        
        if (strcmp(type, "Hub") == 0 || strcmp(type, "Timer") == 0){
            if(send_ipc_message(id, "self_info") == FAILURE){
                perror("error writing to self pipe\n");
                return;
            }
        } else {
            char* parent = strtok(NULL, ", "); // parent
            if(strcmp(parent, "0") == 0){
                if(send_ipc_message(id, "self_info") == FAILURE){
                    perror("error writing to child pipe\n");
                    return;
                }
            } else {
                char message[MSG_SIZE];
                snprintf(message, sizeof(message), "child_info %s", id);
                if(send_ipc_message(parent, message) == FAILURE){
                    perror("error writing to parent pipe\n");
                    return;
                }
            } 
        }

        strcpy(info, "request sent");
        return; 
    } else {
        strcpy(info, "\0");
        printf("Device with id %s does not exist in the registry.\n", id);
        return;
    }
}

//searches the registry for the row corresponding to the device with the given id and returns it, if it doesn't exist returns NULL
void get_device_row(char* id, char* row_copy) {
    FILE *fp = fopen(".registry.txt", "r");
    strcpy(row_copy, "\0"); 
    
    if (fp != NULL) {
        char row[200];
        while (fgets(row, sizeof(row), fp) != NULL) {
            char current_row[200];
            strcpy(current_row, row);
            
            char* current_id = strtok(current_row, ", ");
            if (current_id != NULL && strcmp(current_id, id) == 0) {
                strcpy(row_copy, row);
                fclose(fp);
                return;
            }
        }
        fclose(fp);
        return; 
    } else {
        perror("error in opening registry file\n");
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
        //here we delete the row of the device
        if (current_id != NULL) {
            if (strcmp(current_id, id_to_delete) == 0) {
                continue; 
            }
        //else, we collect the infos 
            char* pid_str = strtok(NULL, ", ");
            char* type_str = strtok(NULL, ", ");
            char* parent_str = strtok(NULL, ", ");
            char* children_str = strtok(NULL, "\n"); 
            
            if (children_str == NULL) {
                children_str = "";        
            }

            //here we check whether the device deleted was a child of a control device, if it is we do not add its id to new_children_str, so that it does not appear in the new registry file
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
        char* type = strtok(NULL, ", "); 
        char* parent_id = strtok(NULL, ", ");
        char msg[MSG_SIZE];
        //check what type of device it is. if control device or interaction device not linked to anything send a self delete, else send a child delete
        if(strcmp(type, "Hub") == 0 || strcmp(type, "Timer") == 0 || strcmp(parent_id, "0") == 0){
            snprintf(msg, sizeof(msg), "self_delete %s", id);
            if (send_ipc_message(id, msg) == SUCCESS) return SUCCESS;
        } else if(strcmp(parent_id, "0") != 0){
            snprintf(msg, sizeof(msg), "child_delete %s", id);
            if (send_ipc_message(parent_id, msg) == SUCCESS) return SUCCESS;
        }
        
        printf("couldn't send the delete command\n");
        return FAILURE;
    }else{
        printf("device row not found in registry \n");
        return FAILURE;
    }
}

void handle_crashed_devices() {
    //collect the pid of the crashed device if there is one
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            //clean process termination
            continue; 
        }
        char id[10];
        memset(id, 0, sizeof(id));
        char row[200];
        memset(row, 0, sizeof(row));
        
        //extract corresponding id and row in registry
        FILE* fp = fopen(".registry.txt", "r");
        if (fp) {
            char temp_row[200];
            while (fgets(temp_row, sizeof(temp_row), fp) != NULL) {
                char row_copy[200];
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
            char* parent_str = strtok(NULL, ", "); 
            char* children_str = strtok(NULL, "\n");

            if (children_str == NULL) {
                children_str = "";
            }

            //if timer or hub it checks whether it had children, and if it did, it links them back to the controller
            if (strcmp(type, "Hub") == 0 || strcmp(type, "Timer") == 0) {
                if (strlen(children_str) > 0) {
                    char children_ids[20][10];
                    int children_count = 0;

                    char* child_token = strtok(children_str, ", ");
                    //now we populate children ids array making sure that we cut out , and spaces or \n
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
                        link_command(children_ids[i], "0");
                    }
                }
            }

                   
            delete_device_from_registry(id);
            
            //remove the named pipe
            char pipename[32];
            snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", id);
            unlink(pipename);
            
            //this part sends a message to the parent of the crashed device telling it to delete it from its children list
            char clean_parent[10];
            memset(clean_parent, 0, sizeof(clean_parent));
            if (parent_str != NULL && sscanf(parent_str, "%s", clean_parent) == 1) {
                if (strcmp(clean_parent, "0") != 0) { 
                    char msg[MSG_SIZE];
                    snprintf(msg, sizeof(msg), "child_delete %s", id);
                    
                    char parent_pipe_name[32];
                    snprintf(parent_pipe_name, sizeof(parent_pipe_name), "/tmp/domotics_%s", clean_parent);
                    int parent_pipe = open(parent_pipe_name, O_WRONLY | O_NONBLOCK);
                    if (parent_pipe != -1) {
                        write(parent_pipe, msg, strlen(msg) + 1);
                        close(parent_pipe);
                    }
                }
            }     
        }
    }
}
