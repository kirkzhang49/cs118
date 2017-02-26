/* Implements sll.h HEADer. For server side of UDP FTP 
 * Brett Chalabian and Jason Woo */

#include "lib.h"
#include <stdio.h>
#include <stdlib.h>

win_list win_listCreate(int win_list_size, int numPKT) {
    win_list init_win;
    init_win.length = 0;
    init_win.max_size = win_list_size;
    init_win.total_PCK = numPKT;
    init_win.cur_pkt = 0;
    init_win.HEAD = NULL;
    init_win.TAIL = NULL;
    return init_win;
}

bool check_all(bool* checklist, unsigned int csize) {
	// check last win_list
	int i;
	if (csize < 100) {
		for (i = 0; i < csize; i++)
			if (checklist[i] == false)
				return false;
	}
	else {
		for (i = csize - 100; i < csize; i++)
			if (checklist[i] == false)
				return false;
	}

	return true;
}
bool Set_Cur_state(win_list* w, int sequenceNum, int state) 
{
    win_list_element* cur = w->HEAD;
    while (cur != NULL) {
        if (cur->packet->seq_num == sequenceNum)
        {
            cur->state = state;
            return true;

        }
        cur = cur->next;
    }
    // Element not found
    return false;
}


void Merror(char *msg)
{
    perror(msg);
    exit(0);
}

bool Doack(win_list* w, int sequenceNum) 
{
    return Set_Cur_state(w, sequenceNum, ACKED);
}

void win_listclean(win_list* w) 
{
    // If the HEAD of the win_list can be recycled, it recycles the HEAD and sets the win_list
    while (w->HEAD != NULL && w->HEAD->state == ACKED) {
        w->length--;
        win_list_element* temp = w->HEAD;

        if (w->HEAD != w->TAIL) 
            w->HEAD = w->HEAD->next;
        else {
            w->HEAD = NULL;
            w->TAIL = NULL;
        }
        free(temp);
    }
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    win_list_element* cur = w->HEAD;

    while (cur != NULL) {
        if (cur->state != ACKED && cur->state != UNSEND) {
            if (tv.tv_sec > cur->tv.tv_sec || (tv.tv_sec == cur->tv.tv_sec && tv.tv_usec > cur->tv.tv_usec)) {
                cur->state = RESEND;
                cur->packet->type = RETRANS;
                printf("Packet number %d has timed out.\n", cur->packet->seq_num);
            }
        }
        cur = cur->next;
    }

}

bool addEle(win_list* w, packet* packet)
{
    if (w->length == w->max_size)
        return false;

    if (w->cur_pkt >= w->total_PCK)
        return false;

    w->length++;
    w->cur_pkt++;
    win_list_element* nelement = malloc(sizeof(win_list_element));
    nelement->state = UNSEND;
    nelement->packet = packet;
    nelement->next = NULL;
    
    if (w->HEAD == NULL && w->TAIL == NULL) {
        w->HEAD = nelement;
        w->TAIL = nelement;
    }
    else {
        w->TAIL->next = nelement;
        w->TAIL = nelement;
    } 
   
    return true; 
}

win_list_element* getEle(win_list w)
{
    win_list_element* cur = w.HEAD;
    while (cur != NULL)
    {
        if (cur->state == UNSEND || cur->state == RESEND)
        {
            return cur;
        }

        cur = cur->next;
    }
    return NULL;
}















