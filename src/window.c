#include "window.h"

window createWindow(){
    window window;
    window.state = 0;
    window.switches = 1;
    window.time_open = 0;
    return window;
}