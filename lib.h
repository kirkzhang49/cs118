#ifndef LIB
#define LIB

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "packet.h"


// Generates a new win_list struct
win_list win_listCreate(int win_list_size, int numPKT);

bool check_all(bool* checklist, unsigned int csize);
// Marks a packet as acknowledged. Should be used when parsing client messages.
bool Doack(win_list* w, int sequenceNum);

// Marks packet as needing to be resent. Should be used when parsing cient messages.
bool resendwin_listElement(win_list* w, int sequenceNum);

// Removes elements from the HEAD of the win_list if they are sent and acknowledged.
void win_listclean(win_list* w);

// Add an element to the win_list. If the win_list is full, it will return false.
bool addEle(win_list* w, packet* packet);

// Get first win_list element that needs to be sent.
win_list_element* getEle(win_list w);

/* Other auxiliary functions */
//bool shouldReceive(float pL, float pC);
void Merror(char *msg);
#endif