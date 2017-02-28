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
	#include <fcntl.h> 
	#include <unistd.h> 
	#include "lib.c"
	#include "packet.h"

	int main(int argc, char *argv[])
	{
		//initialize 
		srand(time(NULL));
		int sockfd, portno;
		socklen_t clilen;
		struct sockaddr_in serv_addr, cli_addr;

		char buffer[PACKET_SIZE];

		if (argc < 2) {
			fprintf(stderr,"Not correct input args\n");
			exit(1);
		}
		memset((char *) &serv_addr, 0, sizeof(serv_addr));	
		portno = atoi(argv[1]);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(portno);

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);	
		if (sockfd < 0) 
			Merror("Merror opening socket");

		if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
			Merror("Merror on binding");

			bzero((char*) &cli_addr, sizeof(cli_addr));
			clilen = sizeof(cli_addr);		

			printf("Waiting for Connection!\n\n");
			bzero((char*) buffer, sizeof(char) * PACKET_SIZE);

		while (1) {//main loop 
			if (recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &cli_addr, &clilen) != -1) { 
				//for file 
				int Nlenth = strlen(buffer);
				char Fname[Nlenth + 1];
				bzero(Fname, sizeof(char) * (Nlenth + 1));
				Fname[Nlenth] = '\0';
				strncpy(Fname, buffer, Nlenth);
				
				bzero((char*) buffer, sizeof(char) * PACKET_SIZE);

				printf("Receiver wants the file: %s\n\n", Fname);

				//	open file  
				FILE* Rfile =  fopen(Fname, "r");
				if (Rfile == NULL) {  //file not found 
					packet Warning;
					Warning.total_size = 0;
					Warning.type = NOTFOUND;
					printf("Can't opening requested file\n");
					sendto(sockfd, (char *) &Warning, sizeof(char) * PACKET_SIZE,
					0, (struct sockaddr*) &cli_addr, sizeof(cli_addr)); //send to notify not found file
					continue;
				}

				int BytesFile = GetFileSize(Fname);//read file and get size
				unsigned int win_list_size =5; //initial window size 
				unsigned int current_win_ele = 0;
				time_t time_s = 0;
				suseconds_t time_us = 500000;

				char* filebuffer = (char*) calloc(BytesFile, sizeof(char));
				if (filebuffer == NULL) {
					Merror("can't allocating space for file");
				}

				fread(filebuffer, sizeof(char), BytesFile, Rfile);
				fclose(Rfile);//done with file 
				//prepare for packets and window elements 
				int packet_nums = (BytesFile % PACK_CONTAIN)!=0 ? (BytesFile/PACK_CONTAIN +1): (BytesFile/PACK_CONTAIN);
				
				packet* My_packets;
				My_packets = (packet *) calloc(packet_nums, PACKET_SIZE);

				win_list w = win_listCreate(win_list_size, packet_nums);
					int checkRest =(BytesFile % PACK_CONTAIN);

				for (int i = 0; i < packet_nums; i++) {
					bool checkEnd =(checkRest!=0 &&(i == packet_nums - 1));
					if (checkEnd)
						strncpy(My_packets[i].buffer, filebuffer + i * PACK_CONTAIN, checkRest);
					else 
						strncpy(My_packets[i].buffer, filebuffer + i * PACK_CONTAIN, PACK_CONTAIN);
		
				int bufferEnd = checkEnd ? checkRest:PACK_CONTAIN;
					My_packets[i].buffer[bufferEnd]	= '\0';
					My_packets[i].total_size = BytesFile;
					My_packets[i].seq_num = (i % SEQMAX)*1024;
					My_packets[i].type = PACKETSEND;
					if (addEle(&w, (My_packets + current_win_ele)))current_win_ele++;//for windows 
				}
		
		
			int sentNum =0;
			while (1) {  //looping here 
				win_list_element* my_win = getEle(w);
				while (my_win != NULL) //sending loop
				{
					unsigned int cs_num = my_win->packet->seq_num;
					sendto(sockfd, (char *) my_win->packet, sizeof(char) * PACKET_SIZE, 0,
					(struct sockaddr*) &cli_addr, sizeof(cli_addr));	
					if (my_win->state == RESEND) 
						{
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
							my_win->state = SENT;
							my_win->tv.tv_sec = T_s;
							my_win->tv.tv_usec = T_us;
							my_win = getEle(w);
					}
					bool Check_rec = true;
					while (Check_rec) {//ACK receive loop
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
									if (packet_nums == current_win_ele + 1 && w.length == 0) {
									printf("Receiving packet %d\n", latest_ACK_received);
										break;//end the loop here 
									}
								printf("Receiving packet %d\n", latest_ACK_received);
								}
							}
						} // End of if recv ACK

						win_listclean(&w); //update window with time out 
						while (current_win_ele != packet_nums && addEle(&w, (My_packets + current_win_ele)))
							current_win_ele++; //adjust window elements
					} 

					bzero((char*) buffer, sizeof(char) * PACKET_SIZE);
			} 
		}

		close(sockfd);//closed the socket 
		return 0;

	}
	