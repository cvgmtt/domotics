#ifndef __BULB_H__
#define __BULB_H__
#include "return_code.h"

typedef struct bulb{
    int state;
    int switches;
    int time;
    int id;
}bulb;

bulb createBulb();
int createProcessBulb(int num);
#endif