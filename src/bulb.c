#include "bulb.h"

bulb createBulb(){
    bulb bulb;
    bulb.state = 0;
    bulb.switches = 1;
    bulb.time = 0.0;
    return bulb;
}