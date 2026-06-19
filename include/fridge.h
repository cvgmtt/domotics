#ifndef __FRIDGE_H__
#define __FRIDFE_H__
#include "return_code.h"
#include "device.h"

typedef struct registry_fridge{
    int time_open;
    int perc; //fill percentage
    int temp;
    int thermostat; //target temperature
    int id;
    int parent_id;
}registry_fridge;

typedef struct fridge{
    int state;
    int switches;
    registry_fridge registry;
}fridge;



fridge createFridge(int _perc, int _temp, int _thermostat);
int createProcessFridge(int num);

#endif