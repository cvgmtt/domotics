#ifndef __FRIDGE_H__
#define __FRIDFE_H__
#include "return_code.h"


typedef struct fridge{
    int state;
    int switches;
    int time_open;
    int perc; //fill percentage
    int temp;
    int thermostat; //target temperature

}fridge;

fridge createFridge(int _perc, int _temp, int _thermostat);
int createProcessFridge();

#endif