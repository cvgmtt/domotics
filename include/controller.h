#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include "return_code.h"
#include "hub.h"
#include "fridge.h"
#include "window.h"
#include "timer.h"
#include "bulb.h"

typedef struct registry{
    int num;
    int id;
}registry;

typedef struct controller{
    int state;
    int switches; 
    registry registry;
} controller;





int list(char* controller_pid_string);

int link_command(char* child_id, char* parent_id);



#endif