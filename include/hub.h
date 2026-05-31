#ifndef __HUB_H__
#define __HUB_H__
#include "return_code.h"


typedef struct hub{
    int state;
    int switches;
    registry registry;
}hub;

typedef struct registry{
    int id;
    int child_switches[]; 
}registry;

hub createHub();
int createProcessHub(int num);

#endif