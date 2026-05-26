#ifndef __WINDOW_H__
#define __WINDOW_H__
#include "return_code.h"

typedef struct window{
    int state;
    int switches;
    int time_open;
    int id;
}window;

window createWindow();
int createProcessWindow(int num);

#endif