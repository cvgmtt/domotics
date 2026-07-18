#ifndef __TIMER_H__
#define __TIMER_H__
#include "return_code.h"
#include "device.h"

typedef struct registry_timer{
    int begin_minutes;
    int end_minutes;
    int id;
    int parent_id;
    int child_id;
}registry_timer;

typedef struct timer{
    int state;
    int switches;
    registry_timer registry;
}timer;


char* self_info_command(timer* current_timer);
char* registry_info(timer* current_timer);
char* child_info_command(char* pipename_child, char* child_id);
timer createTimer();
int createProcessTimer(int num);

#endif