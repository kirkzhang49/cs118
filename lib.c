/* Implements sll.h HEADer. For server side of UDP FTP 
 * Brett Chalabian and Jason Woo */

#include "lib.h"
#include <sys/time.h>
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

		for (int i = 0; i < csize; i++)
		{	
            if (checklist[i] == false)
				return false;
        }
	return true;
}

void Merror(char *msg)
{
    perror(msg);
    exit(0);
}

bool Doack(win_list* w, int sequenceNum) 
{
     win_list_element* Current = w->HEAD;
    while (Current != NULL) {
        if (Current->packet->seq_num == sequenceNum)
        {
            Current->state = ACKED;
            return true;

        }
        Current = Current->next;
    }
    return false;
}

void win_listclean(win_list* w) 
{
    while (w->HEAD != NULL && w->HEAD->state == ACKED) {
        win_list_element* temp = w->HEAD;
       w->length--;
    
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
                printf("Packet  %d: timedout.\n", cur->packet->seq_num);
            }
        }
        cur = cur->next;
    }

}

bool addEle(win_list* w, packet* packet)
{
    if (w->length == w->max_size||w->cur_pkt >= w->total_PCK)
        return false;

    w->length++;
    w->cur_pkt++;
    win_list_element* NextELE = malloc(sizeof(win_list_element));
    NextELE->packet = packet;
    NextELE->next = NULL;
    NextELE->state = UNSEND;

    
    if (w->HEAD == NULL && w->TAIL == NULL) {
        w->HEAD = NextELE;
        w->TAIL = NextELE;
    }
    else {
        w->TAIL->next = NextELE;
        w->TAIL = NextELE;
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
long GetFileSize(const char* filename)
{
    long size;
    FILE *f;
 
    f = fopen(filename, "rb");
    if (f == NULL) return -1;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fclose(f);
 
    return size;
}


void writeToFile(char* fileContent,int file_size)
{
	fileContent[file_size] = '\0';
	FILE* f = fopen("test.txt", "wb");
	if (f == NULL) {
		Merror("error with opening file");
	}
	fwrite(fileContent, sizeof(char), file_size, f);
	fclose(f);
}













