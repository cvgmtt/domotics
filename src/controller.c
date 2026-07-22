#include "controller.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>

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

    int max_fd = controller_pipe > STDIN_FILENO ? controller_pipe : STDIN_FILENO;
    fd_set read_fds;

    while(1){
        printf("Enter the desired command: ");
        fflush(stdout);

        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(controller_pipe, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            continue; 
        }

        if (FD_ISSET(controller_pipe, &read_fds)) {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            int bytes_read = read(controller_pipe, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                if (strncmp(buffer, "del ", 4) == 0) {
                    char* id_to_del = buffer + 4;
                    delete_device_from_registry(id_to_del);
                    printf("\nDevice %s deleted successfully.\n", id_to_del);
                } else {
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
                    char* id = strtok(NULL, " ");
                    printf("id: %s\n", id);
                    if(id != NULL){
                        char* label = strtok(NULL, " ");
                        printf("label: %s\n", label);
                        char* pos = strtok(NULL, ", ");
                        if(strcmp(label, "power") != 0 && strcmp(label, "door")!= 0 && strcmp(label, "window") != 0){
                            printf("wrong input. switch requires a valid label such as: door, window, power\n");
                        }else if(strtok(NULL, " ") == NULL){
                                switch_device(id, label, pos);
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
                        char info[512];
                        info[0] = '\0'; 
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

void switch_device(char* id, char* label, char* pos){
    //get the row of the device
    char pipename[30];
    char row[200];
    char message[50];
    get_device_row(id, row);
    //check for valid row
    if(strcmp(row, "\0") == 0){
        return; //id not found
    }
    //copy row to extract parametres we need
    char row_copy[200];
    strcpy(row_copy, row);
    strtok(row_copy, ", "); // id
    strtok(NULL, ", "); // pid
    char* type = strtok(NULL, ", ");
    char* parent_id = strtok(NULL, ", ");

    //check if label is consisent with type of device
    if(strcmp(label, "power") == 0){
        //check if is an interaction device or a bulb
        if(strcmp(type, "Timer") == 0 || strcmp(type, "Hub") == 0){
            //send the switch command to the device
            snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", id);
            int device_pipe = open(pipename, O_WRONLY | O_NONBLOCK);
            if(device_pipe >= 0){
                snprintf(message, sizeof(message), "switch %s", pos);
                write(device_pipe, message, strlen(message) + 1);
                close(device_pipe);
                //debug
                printf("sent message to control device\n");
            } else{
                printf("couldn't open the pipe of the device to switch it 1\n");
            }
        } else if(strcmp(type, "Bulb") == 0){
            //send the switch command to the father of the device
            //debug
            printf("value of parent: %s\n", parent_id);
            if(parent_id == NULL){
                printf("couldn't find parent for Bulb id %s\n", id);
                return;
            }
            snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", parent_id);
            int device_pipe = open(pipename, O_WRONLY | O_NONBLOCK);
            if(device_pipe >= 0){
                snprintf(message, sizeof(message), "switch_child %s %s", id, pos);
                write(device_pipe, message, strlen(message) + 1);
                close(device_pipe);
                //debug
                printf("send messato to parent of bulb\n");
            } else{
                printf("couldn't open the pipe of the device to switch it 2\n");
            }
        }else{
            printf("inconsistent label and type of device, cannot perfrom action\n");
        }
    }else if (strcmp(label, "door") == 0 && strcmp(type, "Fridge") == 0){
        //send the switch command to the father of the device
        //debug
        printf("value of parent: %s\n", parent_id);
        if(parent_id == NULL){
            printf("couldn't find parent for Fridge id %s\n", id);
            return;
        }
        snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", parent_id);
        int device_pipe = open(pipename, O_WRONLY | O_NONBLOCK);
        if(device_pipe >= 0){
            snprintf(message, sizeof(message), "switch_child %s %s", id, pos);
            write(device_pipe, message, strlen(message) + 1);
            close(device_pipe);
            //debug
            printf("send messato to parent of fridge\n");
        } else{
            printf("couldn't open the pipe of the device to switch it 4\n");
        }
    }else if (strcmp(label, "window") == 0 && strcmp(type, "Window") == 0){
        //send the switch command to the father of the device
        //debug
        printf("value of parent: %s\n", parent_id);
        if(parent_id == NULL){
            printf("couldn't find parent for Window id %s\n", id);
            return;
        }
        snprintf(pipename, sizeof(pipename), "/tmp/domotics_%s", parent_id);
        int device_pipe = open(pipename, O_WRONLY | O_NONBLOCK);
        if(device_pipe >= 0){
            snprintf(message, sizeof(message), "switch_child %s %s", id, pos);
            write(device_pipe, message, strlen(message) + 1);
            close(device_pipe);
            //debug
            printf("send messato to parent of window\n");
        } else{
            printf("couldn't open the pipe of the device to switch it 5\n");
        }
    }else{
        printf("inconsistent label and type of device, cannot perfrom action\n");
    }
    return;
};

void list(char* list_info){
    FILE *fp = fopen(".registry.txt", "r");
    //checks if file is not empty
    if (fp != NULL) {
        //debug
        printf("registry aperto\n");
        char row[200];
        char info[512] = "\0";
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
    

    char buffer[50];
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
                    write(child_pipe, buffer, strlen(buffer) + 1);
                    close(child_pipe);
                } else{
                    memset(buffer, 0, sizeof(buffer));
                    snprintf(buffer, sizeof(buffer), "new parent %s %s", parent_id, child_id);
                    snprintf(pipename_old_parent, sizeof(pipename_old_parent), "/tmp/domotics_%s", parent_to_change);
                    int old_parent_pipe = open(pipename_old_parent, O_WRONLY | O_NONBLOCK);
                    if(old_parent_pipe >= 0){
                        write(old_parent_pipe, buffer, strlen(buffer) + 1);
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
                    write(parent_pipe, buffer, strlen(buffer) + 1);
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
            if (write(self_pipe, "self_info", strlen("self_info") + 1) < 0) {
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
                if (write(child_pipe, "self_info", strlen("self_info") + 1) < 0) {
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
                char message[30];
                snprintf(message, sizeof(message), "child_info %s", id);
                if (write(parent_pipe, message, strlen(message) + 1) < 0) {
                    perror("error writing to parent pipe");
                    close(parent_pipe);
                    return;
                }
                printf("message sent\n");
                close(parent_pipe);
            } 
        }
        //read response from the pipe of controller and return it
        char response[512]; 
        int controller_pipe = open(controller_pipename, O_RDONLY);
        if (controller_pipe < 0) {
            perror("error opening controller pipe");
            return;
        }
        ssize_t bytes_read = read(controller_pipe, response, sizeof(response) - 1);
        printf("reading response\n");
        if (bytes_read >= 0) {
            response[bytes_read] = '\0'; // Null-terminate the string
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

    strcpy(row_copy, "\0"); // Initialize row_copy to an empty string

    //checks if file is not empty
    if (fp != NULL) {
        //debug
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
        return; // id not found
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
        strtok(row_copy, ", "); // id
        strtok(NULL, ", "); // pid
        char pipename[20];
        char* type = strtok(NULL, ", "); // type
        char* parent_id = strtok(NULL, ", ");
        char msg[20];
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
