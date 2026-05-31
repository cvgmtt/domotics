#ifndef __BULB_H__
#define __BULB_H__
#include "return_code.h"

typedef struct bulb{
    int state;
    int switches;
    registry registry;
}bulb;

typedef struct registry{
    int time;
    int id;
}registry;

bulb createBulb();
int createProcessBulb(int num);
#endif