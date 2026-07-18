#ifndef __WINDOW_H__
#define __WINDOW_H__
#include "return_code.h"
#include "device.h"

typedef struct registry_window{
    int time_open;
    int id;
    int parent_id;
}registry_window;

typedef struct window{
    int state;
    int switches;
    registry_window registry;
}window;


char* self_info_command(window* current_window);
window createWindow();
int createProcessWindow(int num);

#endif