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

FILE* initDevice(int fd[], int pipe);

int checkSuccess(int fd[], pid_t pid);

int createPipe(int num, char* pipename, size_t size);

void child_info_command(char* pipename_child, char* pipename_parent, char* response, size_t size);

int getCommand(char* buf, char* id, char* pos, char* child_id);


#endif