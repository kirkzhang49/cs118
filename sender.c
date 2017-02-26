#include <stdio.h>
#include <sys/types.h>   
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	
#include <signal.h>	
#include <sys/stat.h> 
#include <time.h>

#include "lib.c"
#include "packet.h"



int main(int argc, char *argv[])
{
	srand(time(NULL));
	int sockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	char buffer[PACKET_SIZE];

	if (argc < 2) {
		 fprintf(stderr,"Merror, no port provided\n");
		 exit(1);
	}

	//reset memory
	memset((char *) &serv_addr, 0, sizeof(serv_addr));	

	//fill in address info
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	//create UDP socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);	
	if (sockfd < 0) 
		Merror("Merror opening socket");

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	    Merror("Merror on binding");

	bzero((char*) &cli_addr, sizeof(cli_addr));
 	clilen = sizeof(cli_addr);		

 	printf("Waiting for Connection!\n\n");
	bzero((char*) buffer, sizeof(char) * PACKET_SIZE);

 	while (1) {
 		// once my_win receive file request from receiver, my_win go in. Otherwise, loop to keep listening
	 	if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &cli_addr, &clilen) != -1) {
		 	int Nlenth = strlen(buffer);
		 	char Fname[Nlenth + 1];
			bzero(Fname, sizeof(char) * (Nlenth + 1));
		 	Fname[Nlenth] = '\0';
		 	strncpy(Fname, buffer, Nlenth);

		 	bzero((char*) buffer, sizeof(char) * PACKET_SIZE);

		 	printf("Receiver wants the file: %s\n\n", Fname);

			//	Get from file 
			FILE* f =  fopen(Fname, "r");
			if (f == NULL) {  //file not found 
				packet notfound;
				notfound.total_size = 0;
				notfound.type = NOTFOUND;

				printf("Can't opening requested file\n");
				sendto(sockfd, (char *) &notfound, sizeof(char) * PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)); //send to notify not found 
				continue;
			}

			// number of bytes in the file
			struct stat st;
			stat(Fname, &st);
			int BytesFile = st.st_size;

			char* ContentFl = (char*) calloc(BytesFile, sizeof(char));
			if (ContentFl == NULL) {
				Merror("can't allocating space for file");
			}

			fread(ContentFl, sizeof(char), BytesFile, f);
			fclose(f);

			/* 	BREAK DOWN FILE */

			int packet_nums = BytesFile / PACK_CONTAIN;
			int remainB = BytesFile % PACK_CONTAIN;
			if (remainB) packet_nums++;//for extra packet
		

			printf("Packet size of file %s: %i\n", Fname, packet_nums);

			/*

				PACKET ARRAY CONSTRUCTION

			*/
			//packet My_packets[packet_nums];
			packet* My_packets;
			My_packets = (packet *) calloc(packet_nums, PACKET_SIZE);

			//printf("Packet Array initialized\n");

			int i;
			for (i = 0; i < packet_nums; i++) {

				if (remainB&&(i == packet_nums - 1)) { 
				// the last packet with remainder bytes doesn't take full space
					strncpy(My_packets[i].buffer, ContentFl + i * PACK_CONTAIN, remainB);
					My_packets[i].buffer[remainB] = '\0';
				}
				else { // normal cases
					strncpy(My_packets[i].buffer, ContentFl + i * PACK_CONTAIN, PACK_CONTAIN);
					My_packets[i].buffer[PACK_CONTAIN]	= '\0';
				}

				My_packets[i].total_size = BytesFile;
				My_packets[i].seq_num = (i % SEQMAX)*1024;
				My_packets[i].type = PACKETSEND;
			}

			//printf("Created array of packets with %i packets\n", packet_nums);


			/*
		
				START SENDING PACKETS

			*/

			unsigned int win_list_size =5; //initial window size 
			unsigned int current_win_ele = 0;

			time_t time_s = 0;
			suseconds_t time_us = 500000;


			// keep sending and receiving ACK until my_win get ACK for last packet
			//while (latest_ACKd_packet != packet_nums - 1) {

				/*
					Send packets within the win_list size					
				*/

				win_list w = win_listCreate(win_list_size, packet_nums);

				while (addEle(&w, (My_packets + current_win_ele))) {
					current_win_ele++;
				}

					// here, packet_to_send will be one greater than last_win_list_packet

				/*
					Loop to receive ACKs within the win_list
				*/
                int sentNum =0;
				while (1) {

					win_list_element* my_win = getEle(w);
					while (my_win != NULL) {
						unsigned int cs_num = my_win->packet->seq_num;
						sendto(sockfd, (char *) my_win->packet, sizeof(char) * PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, sizeof(cli_addr));	
						if (my_win->state == RESEND) {
							printf("Sending packet  %d %dRetransmission\n", cs_num,win_list_size*PACKET_SIZE);
						}
						if (my_win->state == UNSEND) {
                            if (sentNum ==0)
                            {
                                printf("Sending packet  %d %d SYN\n", cs_num,win_list_size*PACKET_SIZE);
                            }
                            else if (sentNum ==packet_nums -1)
							printf("Sending packet  %d %d FIN\n", cs_num,win_list_size*PACKET_SIZE);
                            else 
                            printf("Sending packet  %d %d\n", cs_num,win_list_size*PACKET_SIZE);
						}
                        sentNum++;

						struct timeval tv;
						gettimeofday(&tv, NULL);
						time_t T_s = tv.tv_sec + time_s;
						suseconds_t T_us = tv.tv_usec + time_us;

						my_win->tv.tv_sec = T_s;
						my_win->tv.tv_usec = T_us;
						my_win->state = SENT;
						my_win = getEle(w);
					}

					// Again, loop to listen for ACK msg
					bool Check_rec = true;
					while (Check_rec) {
						Check_rec = false;
						if (recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*) &cli_addr, &clilen) != -1) {
							Check_rec = true;
							// TODO: handle packet corruption & loss

							packet* ACK_message = (packet *) buffer;

							if (ACK_message == NULL) {
								Merror("nothing in ACK_message");
							}

							int latest_ACK_received = -1;
							if (ACK_message->type == ACKEDP)
								latest_ACK_received = ACK_message->seq_num;
							
							if (Doack(&w, latest_ACK_received)) {

								printf("Receiving packet %d\n", latest_ACK_received);


								// if the first win_list element is ACK'd, my_win can slide win_list
								if (packet_nums == current_win_ele + 1 && w.length == 0) {
									printf("Receiving packet %d\n", latest_ACK_received);
									break;
								}

							}

							

						}
					} // End of if recv ACK

					win_listclean(&w); 
					while (current_win_ele != packet_nums && addEle(&w, (My_packets + current_win_ele)))
						current_win_ele++;

				/*	if (packet_nums == current_win_ele && w.length == 0) {
						printf("Receiving packet done\n");
						break;
					}*/

				} // End of ACK while loop
				//printf("Done with file transfer\n\n");



				bzero((char*) buffer, sizeof(char) * PACKET_SIZE);
		} // end of if(recvfrom)
	}

 	close(sockfd);
 	return 0;

}
 