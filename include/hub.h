#ifndef __HUB_H__
#define __HUB_H__
#include "return_code.h"


typedef struct hub{
    int state;
    int switches;
    int registry[]; 
}hub;

hub createHub();
int createProcessHub();

#endif