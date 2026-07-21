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


void window_info_command(window* current_window, char* info, size_t size);
window createWindow();
int createProcessWindow(int num);

#endif