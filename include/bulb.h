#ifndef __BULB_H__
#define __BULB_H__
#include "return_code.h"
#include <device.h>

typedef struct registry_bulb{
    int time;
    int id;
}registry_bulb;

typedef struct bulb{
    int state;
    int switches;
    registry_bulb registry;
}bulb;



bulb createBulb();
int createProcessBulb(int num);
#endif