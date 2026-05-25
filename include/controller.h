#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include "return_code.h"


typedef struct controller{
    int state;
    int switches; 
    int num;
    int process_id;
} controller;

int list(char* controller_pid_string);


#endif