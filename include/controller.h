#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include "return_code.h"
#include "hub.h"
#include "fridge.h"
#include "window.h"
#include "timer.h"
#include "bulb.h"

typedef struct controller{
    int state;
    int switches; 
    int num;
    int process_id;
    int id;
} controller;

int list(char* controller_pid_string);


#endif