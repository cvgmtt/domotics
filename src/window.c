#include "window.h"
#include "time.h"
#include "return_code.h"

window createWindow(){
    window window;
    window.state = 0;
    window.switches = 1;
    window.time_open = 0;
    return window;
}

int createProcess(){
    window window = createWindow();
    pid pid = fork();
    if(pid < 0){
        return FAILURE;
    } else if(pid == 0){
        while(1){
            
        }
    }
}