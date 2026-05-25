#include "bulb.h"

bulb createBulb(){
    bulb bulb;
    bulb.state = 0;
    bulb.switches = 1;
    bulb.time = 0.0;
    return bulb;
}

int createProcess(){
    bulb bulb = createBulb();
    pid pid = fork();
    if(pid < 0){
        return FAILURE;
    } else if(pid == 0){
        while(1){
            
        }
    }
}