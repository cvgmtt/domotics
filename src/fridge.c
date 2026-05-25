#include "fridge.h"

fridge createFridge(int _perc, int _temp, int _thermostat){
    fridge fridge;
    fridge.state = 0;
    fridge.switches = 1;
    fridge.time_open = 0;
    fridge.perc = _perc;
    fridge.temp = _temp;
    fridge.thermostat = _thermostat;
    return fridge;

}

int createProcess(){
    fridge fridge = createFridge();
    pid pid = fork();
    if(pid < 0){
        return FAILURE;
    } else if(pid == 0){
        while(1){
            
        }
    }
}