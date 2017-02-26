#ifndef PACKET
#define PACKET

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define PACKET_SIZE 1024
#define PACK_CONTAIN (PACKET_SIZE - sizeof(long) - 3*sizeof(int) - 1)

#define SEQMAX 30

#define ACKEDP 'a'
#define RETRANS 'r'
#define PACKETSEND 's'
#define NOTFOUND 'n'
#define REQUESTED 'f'

#define UNSEND 0
#define RESEND 1
#define SENT 2
#define ACKED  3

typedef struct packet {
    char type;
	char buffer[PACK_CONTAIN + 1];
    unsigned long total_size;
	unsigned int seq_num;
} packet;

typedef struct win_list_element {
    struct win_list_element* next;
    packet* packet;
    int state; 
    struct timeval tv;
} win_list_element;

typedef struct {
  
    int length;
    int max_size;
    int total_PCK;
    int cur_pkt;
    win_list_element* HEAD;
    win_list_element* TAIL;
} win_list;

#endif
