#include "hub.h"

hub createHub(){
    hub hub;
    hub.state = 1; //capisci se va bene
    hub.switches = 1;
    hub.registry[5];
    return hub;
}

int createProcess(){
    hub hub = createHub();
    pid pid = fork();
    if(pid < 0){
        return FAILURE;
    } else if(pid == 0){
        while(1){
            
        }
    }
}