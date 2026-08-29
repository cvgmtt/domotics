#ifndef __TIMER_H__
#define __TIMER_H__
#include "return_code.h"
#include "device.h"

typedef struct registry_timer{
    char begin_time[26];
    char end_time[26];
    int id;
    int parent_id;
    int child_id;
    int child_state;
}registry_timer;

typedef struct timer{
    int state;
    int switches;
    registry_timer registry;
}timer;


void timer_info_command(timer* current_timer,  char* info, size_t size);
void timer_registry_info(timer* current_timer,  char* info, size_t size);
timer createTimer();
int createProcessTimer(int num);

#endif