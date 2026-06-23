#include "controller.h"
#include "string.h"
#include "stdio.h"
#include <stdio.h>




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

    while(1){
        char terminal_input[30];
        printf("Enter the desired command: ");
        fgets(terminal_input, 30, stdin);
        terminal_input[strcspn(terminal_input, "\n")] = '\0';
        char *token = strtok(terminal_input, " ");
        pid_t controller_pid = getpid();
        char controller_pid_string[16];   
        sprintf(controller_pid_string, "%d", controller_pid);        
        if(token != NULL){
            if(strcmp(token, "list") == 0){
                if(list(controller_pid_string) == FAILURE){
                    perror("could not open file");
                };
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
                    //iterates through ids till you find it
                    //then delete it
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
                    //iterates through ids till you find the correct ones, if you don't find them, throw an error
                } else{
                    printf("wrong input. info requires a valid id\n");
                }
            
            } else{
                printf("please, provide one of these commands:\n");
                printf("list, add <device>, del <id>, link <id1> to <id2>, switch <id> <label> <pos>, info <id>\n");                
            } 
        } else{
            printf("please, provide one of these commands:\n");
            printf("list, add <device>, del <id>, link <id1> to <id2>, switch <id> <label> <pos>, info <id>\n");
        }
    };
}

int list(char* controller_pid_string){
    FILE* fp = fopen(".registry.txt", "r");
    if(fp == NULL){
        return FAILURE;
    }
    char data[50];
    while (fgets(data, 50, fp) != NULL){

        int id, pid;
        char device[50];
        
        sscanf(data, "%d , %d , %s", &id, &pid, device); 
        printf("Id: %d Device: %s\n", id, device);

    }
    fclose(fp);
    return SUCCESS;
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
                    perror("first id is not an interaction device \n");
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
//function to find the control device in case an interaction device has one and to check wether the control device selected is full or not
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