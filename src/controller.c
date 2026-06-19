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
                        if(link_command(child_id, parent_id) == SUCCESS){
                            printf("Linked %s with %s \n", child_id, parent_id);
                        }else{
                            printf("couldn't link these two");
                        };
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
    FILE* fp = fopen(".registry.txt", "r");
    FILE* temp = fopen("temp.txt", "w");
    char pipename_child[20];
    char pipename_parent[20];
    snprintf(pipename_child, sizeof(pipename_child), "/tmp/domotics_%s", child_id);
    snprintf(pipename_parent, sizeof(pipename_parent), "/tmp/domotics_%s", parent_id);
    int child_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
    int parent_pipe = open(pipename_child, O_WRONLY | O_NONBLOCK);
    

    char buffer[15];
    if (fp == NULL || temp == NULL || child_pipe < 0 || parent_pipe < 0) {
        printf("error in linking \n");
        return FAILURE;
    }

    char row[256];
    
    while (fgets(row, sizeof(row), fp) != NULL) {
        char row_copy[256];
        strcpy(row_copy, row); 
        
        char* current_id = strtok(row_copy, ", ");
        
        if (current_id != NULL) {
            char* pid_str = strtok(NULL, ",");
            char* type_str = strtok(NULL, ",");
            
            char* old_parent_str = strtok(NULL, ",");
            
            char* children_str = strtok(NULL, "\n"); 
            if (children_str == NULL) {
                children_str = ""; 
            }

            if (strcmp(current_id, child_id) == 0) {
                printf("Trovato figlio nel registry, aggiorno il suo Parent ID...\n");
                snprintf(buffer, sizeof(buffer), "new parent %s", parent_id);
                write(child_pipe, buffer, sizeof(buffer));
                close(child_pipe);

                fprintf(temp, "%s, %s, %s, %s, %s \n", current_id, pid_str, type_str, parent_id, children_str);
            } 
            else if (strcmp(current_id, parent_id) == 0) {
                printf("Trovato padre nel registry, aggiungo il figlio...\n");

                snprintf(buffer, sizeof(buffer), "new child %s", child_id);
                write(parent_pipe, buffer, sizeof(buffer));
                close(parent_pipe);

                fprintf(temp, "%s, %s, %s, 0, %s \n", current_id, pid_str, type_str, child_id);
                
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


