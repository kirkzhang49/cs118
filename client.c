#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h> 
#include "lib.c"
#include "packet.h"


int main(int argc, char* argv[]) {
	//srand(time(NULL));
	//initialize 
	int clientsocket; 
    int portno;
    socklen_t len;
    struct sockaddr_in serv_addr;
    struct hostent *server; 
    char buffer[PACKET_SIZE];

	if (argc < 3) {
		 fprintf(stderr,"Wrong input arguments /n");
		 exit(1);
	}
    portno = atoi(argv[2]);
	clientsocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientsocket < 0)
		Merror("Can't opening client socket");

 	bzero((char*) &serv_addr, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
 	serv_addr.sin_port = htons(portno);

 	server = gethostbyname(argv[1]);
 	if (server == NULL) {
 		Merror("Can't getting host");
 	}
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length); 	
	
	printf("Sending request to the sender\n\n");//done with Socket part 

	char* filename = argv[3]; //precessing file 
	printf("Requesting the file: %s\n", filename);
	sendto(clientsocket, filename, strlen(filename) * sizeof(char), 
					0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));


	packet* file_packets = NULL;	// Prepare for packet and timer 
	unsigned long file_size;
	unsigned int received_packets = 0;
	unsigned int total_numPKT;
	bool firstCheck = false;
	time_t cur_time = time(NULL);

	bool* receive_check;
	unsigned int Counter1 = 0;
	bool CheckAdd = false;
	unsigned int Counter2 = 0;
	int C_seq_n = 0;


	while (firstCheck == false)
	 {
		if (time(NULL) - cur_time > 1) {//request 
				cur_time = time(NULL);
				printf("Requesting the file: %s\n", filename);
			sendto(clientsocket, filename, strlen(filename) * sizeof(char), 
				0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	}
		if (recvfrom(clientsocket, buffer, sizeof(buffer), MSG_DONTWAIT,(struct sockaddr*) &serv_addr, &len) != -1 ) 
		{
			firstCheck = true;
			packet* convert_pkt_buf = (packet *) buffer;//convert to packet struct
			packet ACK_packet;

			int Seq_num = (convert_pkt_buf->seq_num)/1024;
			C_seq_n = convert_pkt_buf->seq_num/1024;
			char TypeCheck = convert_pkt_buf->type;
			file_size = convert_pkt_buf->total_size;
			if (C_seq_n - Seq_num > 20 && !CheckAdd) {
				Counter1++;
				Counter2 = 0;
				CheckAdd = true;
				Seq_num += Counter1 * SEQMAX;
			}
			else {

				if (Seq_num > 20 && CheckAdd && Counter2 < 15) {
					Seq_num += (Counter1 - 1) * SEQMAX;
				}
				else {
					Seq_num += Counter1 * SEQMAX;
					if (CheckAdd)
						Counter2++;
					if (Counter2 > 14) {
						CheckAdd = false;
						Counter2 = 0;
					}
				}

			}
		
			if (TypeCheck == NOTFOUND) {
				printf("FILE NOT FOUND. STOP.\n");
				exit(1);
			}
			
			total_numPKT = (file_size / PACK_CONTAIN)==0? (file_size / PACK_CONTAIN):(file_size / PACK_CONTAIN)+1;
			file_packets =  (packet *) calloc(total_numPKT, sizeof(packet));
			if (file_packets == NULL) 
			{Merror("error allocating for receiving file packets");}
			    receive_check = (bool *) calloc(total_numPKT, sizeof(bool));
			file_packets[Seq_num] = *convert_pkt_buf;
			printf("Receiving packet %i \n", Seq_num);
			
			ACK_packet.type = ACKEDP;
			ACK_packet.seq_num = Seq_num;
			ACK_packet.total_size = file_size;
			received_packets++;
			receive_check[Seq_num] = true;

			sendto(clientsocket, (char *) &ACK_packet, PACKET_SIZE, 
				0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			printf("Sending packet %i\n", ACK_packet.seq_num);
			if (check_all(receive_check, total_numPKT)) {
				break;
			}
		}
	}


	while (!check_all(receive_check, total_numPKT)) {
		// keep looping to receive file packets
		if (recvfrom(clientsocket, buffer, sizeof(buffer), 0,(struct sockaddr*) &serv_addr, &len) != -1 )//	&& shouldReceive(pL, pC) 
		{
			packet* convert_pkt_buf = (packet *) buffer;
			packet ACK_packet;
			int Seq_num = (convert_pkt_buf->seq_num)/1024;

			if (C_seq_n - Seq_num > 20 && !CheckAdd) {
				Counter1++;
				Counter2 = 0;
				CheckAdd = true;
				Seq_num += Counter1 * SEQMAX;
			}
			else {
				if (Seq_num > 20 && CheckAdd && Counter2 < 16) {
					Seq_num += (Counter1 - 1) * SEQMAX;
				}
				else {
					Seq_num += Counter1 * SEQMAX;
					if (CheckAdd)
						Counter2++;
					if (Counter2 > 12) {
						CheckAdd = false;
						Counter2 = 0;
					}
				}

			}

			C_seq_n = convert_pkt_buf->seq_num/1024;

			char packetType = convert_pkt_buf->type;
			file_packets[Seq_num] = *convert_pkt_buf;
			printf("Receiving packet %i\n", convert_pkt_buf->seq_num);
		

			received_packets++;
			receive_check[Seq_num] = true;
			ACK_packet.type = ACKEDP;
			ACK_packet.seq_num = convert_pkt_buf->seq_num; 
			ACK_packet.total_size = file_size;

					
			sendto(clientsocket, (char *) &ACK_packet, PACKET_SIZE, //Received file packet, so send an ACK correspondingly
				0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			printf("Sending packet %i", ACK_packet.seq_num);
            	if (packetType == PACKETSEND) {
				printf("\n");
			}
			if (packetType == RETRANS) {
				printf("Retransmission\n");
			}
		}
	} // End of receiving file packets while loop

	char* fileContent;//writing to file output 
	fileContent = (char *) calloc(file_size + 1, sizeof(char));
	//memset(fileContent, 0, file_size + 1);	
	for (int i = 0; i < total_numPKT; i++) {
		strcat(fileContent, file_packets[i].buffer);
	}
	writeToFile( fileContent, file_size);
	printf("ALL DONE\n");

	close(clientsocket);
	exit(1);
	return 0;
}


