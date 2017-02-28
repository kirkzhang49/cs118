#ifndef LIB
#define LIB

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "packet.h"


void Merror(char *msg);

win_list win_listCreate(int win_list_size, int numPKT);

bool check_all(bool* checklist, unsigned int csize);

bool Doack(win_list* w, int sequenceNum);

bool resendwin_listElement(win_list* w, int sequenceNum);

void writeToFile(char* fileContent,int file_size);

long GetFileSize(const char* filename);

void win_listclean(win_list* w);

bool addEle(win_list* w, packet* packet);

win_list_element* getEle(win_list w);

#endif