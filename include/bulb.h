#ifndef __BULB_H__
#define __BULB_H__
#include "return_code.h"
#include <device.h>

typedef struct registry_bulb{
    int time;
    int id;
    int parent_id;
}registry_bulb;

typedef struct bulb{
    int state;
    int switches;
    registry_bulb registry;
}bulb;


char* self_info_command();
bulb createBulb();
int createProcessBulb(int num);
#endif