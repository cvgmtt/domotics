#ifndef __BULB_H__
#define __BULB_H__

typedef struct bulb{
    int state;
    int switches;
    int time;
}bulb;

int createBulb();

#endif