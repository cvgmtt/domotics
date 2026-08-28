#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include "return_code.h"
#include "hub.h"
#include "fridge.h"
#include "window.h"
#include "timer.h"
#include "bulb.h"
#define MSG_SIZE 200

typedef struct registry{
    int num;
    int id;
}registry;

typedef struct controller{
    int state;
    int switches; 
    registry registry;
} controller;



void list(char* list_info);

int switch_device(char* id, char* label, char* pos);

int link_command(char* child_id, char* parent_id);

int check_parents(char* parent_to_change, char* child_id, char* parent_id);

int del_command(char* id);

void delete_device_from_registry(char* id_to_delete);
void get_info(char* id, char* info);

void get_device_row(char* id, char* row_copy);
void sigchld_handler(int sig);
void handle_crashed_devices();

#endif