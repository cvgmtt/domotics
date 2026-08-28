#ifndef __DEVICE_H__
#define __DEVICE_H__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <return_code.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#define MSG_SIZE 200

int checkSuccess(int fd[], pid_t pid);

int createPipe(int num, char* pipename, size_t size);

void child_info_command(char* pipename_child);

int getCommand(char* buf, char* id, char* pos, char* child_id);

void kill_device(int id);

int confirm_del(char* pipename_parent);

void send_info_to_controller(const char* info); 

void delete_interaction_device(int my_id, int parent_id);

int wait_for_device_response(int my_id, char* response_buf, size_t buf_size);

int setup_device(int id, const char* type_name, int control_pipe[]);

void wait_function();

int check_inconsistency(int child_state, int parent_state);

int send_ipc_message(const char* target_id, const char* message); 
#endif