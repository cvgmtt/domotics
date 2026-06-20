#ifndef __HUB_H__
#define __HUB_H__
#include "return_code.h"
#include "device.h"

typedef struct registry_hub{
    int id;
    int child_switches[20]; 
    int parent_id;
    int child_num;
    int child_id[20];
}registry_hub;

typedef struct hub{
    int state;
    int switches;
    registry_hub registry;
}hub;



hub createHub();
int createProcessHub(int num);

#endif