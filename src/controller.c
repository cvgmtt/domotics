#include "controller.h"
#include "string.h"
#include "stdio.h"

int main(){
    //initialization of controller
    controller controller;
    controller.state = 1; //means on
    controller.switches = 1; //means on
    controller.num = 0;

    while(1){
        char terminal_input[30];
        printf("Enter the desired command: ");
        fgets(terminal_input, 30, stdin);
        terminal_input[strcspn(terminal_input, "\n")] = '\0';
        char *token = strtok(terminal_input, " ");
        printf("%c \n", token);
        if(token != NULL){
            if(strcmp(token, "list") == 0){
                printf("list devices \n");
            } else if (strcmp(token, "add") == 0){
                token = strtok(NULL, " ");
                printf("got here \n");
                if(token != NULL){
                    if(strcmp(token, "hub") == 0){
                        printf("spawn hub \n");
                    } else if(strcmp(token, "timer") == 0)  {
                        printf("spawns timer \n");
                    } else if(strcmp(token, "bulb") == 0){
                        printf("spawns bulb\n");
                    } else if(strcmp(token, "window") == 0){
                        printf("spawns window\n");
                    } else if(strcmp(token, "fridge") == 0){
                        printf("spawns fridge\n");
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
                    char* id1 = token;
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        char* id2 = token;
                        //iterates through ids till you find the correct ones, if you don't find them, throw an error
                        //update routing tables/ routing tables
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
