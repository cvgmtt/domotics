#ifndef __WINDOW_H__
#define __WINDOW_H__

typedef struct window{
    int state;
    int switches;
    int time_open;
}window;

window createWindow();
int createProcess();

#endif