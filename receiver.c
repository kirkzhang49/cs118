#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      
#include <stdlib.h>
#include <strings.h>
#include <time.h>

#include "lib.c"
#include "packet.h"


int main(int argc, char* argv[]) {

	int clientsocket; 
    int portno;
    socklen_t len;
    struct sockaddr_in serv_addr;
    struct hostent *server; //contains tons of information, including the server's IP address
    char buffer[PACKET_SIZE];

    // pre-PLPC testing - too lazy to write all the parameters for testing
	if (argc < 3) {
		 fprintf(stderr,"Wrong inputs /n");
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
	
	printf("Sending request to the sender\n\n");


	char* filename = argv[3];
	printf("Requesting the file: %s\n", filename);
	sendto(clientsocket, filename, strlen(filename) * sizeof(char), 
					0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));


	// variables to keep track of packet information
	packet* file_packets = NULL;
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


	// Keeps attempting to send file request until it gets a response. 
	while (firstCheck == false) {
		// Attempts to send a request 
		if (time(NULL) - cur_time > 1) {
				cur_time = time(NULL);
				printf("Requesting the file: %s\n", filename);
				sendto(clientsocket, filename, strlen(filename) * sizeof(char), 
							0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		}

		if (recvfrom(clientsocket, buffer, sizeof(buffer), MSG_DONTWAIT,(struct sockaddr*) &serv_addr, &len) != -1 ) 
		{
			firstCheck = true;
			packet* content_packet = (packet *) buffer;
			packet ACK_packet;

			int Seq_num = (content_packet->seq_num)/1024;
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



			C_seq_n = content_packet->seq_num/1024;
			char packetType = content_packet->type;
			if (packetType == PACKETSEND) {
				printf("Packet type: data packet.\n");
			}
			if (packetType == RETRANS) {
				printf("Packet type: retransmitted data packet.\n");
			}
			if (packetType == NOTFOUND) {
				printf("Packet type: FILE NOT FOUND. Exiting.\n");
				exit(1);
			}


			// Allocates space for file in a file_packet buffer.
			file_size = content_packet->total_size;
			total_numPKT = (file_size / PACK_CONTAIN);
			if (file_size % PACK_CONTAIN) // dangling byte packet
				total_numPKT++;
			file_packets =  (packet *) calloc(total_numPKT, sizeof(packet));
			if (file_packets == NULL) {
				Merror("error allocating for receiving file packets");
			}
			else
				printf("Allocated space for %i packets\n", total_numPKT);

			receive_check = (bool *) calloc(total_numPKT, sizeof(bool));

			// Places the first packet received in the correct position of the file_packets buffer.
			
			file_packets[Seq_num] = *content_packet;
			printf("Receiving packet %i \n", Seq_num);
			

			received_packets++;
			receive_check[Seq_num] = true;
			ACK_packet.type = ACKEDP;
			ACK_packet.seq_num = Seq_num;
			ACK_packet.total_size = file_size;

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
			packet* content_packet = (packet *) buffer;
			packet ACK_packet;

			int Seq_num = (content_packet->seq_num)/1024;

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

			C_seq_n = content_packet->seq_num/1024;

			char packetType = content_packet->type;
			file_packets[Seq_num] = *content_packet;
			printf("Receiving packet %i\n", content_packet->seq_num);
		

			received_packets++;
			receive_check[Seq_num] = true;
			ACK_packet.type = ACKEDP;
			ACK_packet.seq_num = content_packet->seq_num; //Seq_num;// % SEQMAX;
			ACK_packet.total_size = file_size;

			/*
				Received file packet, so send an ACK correspondingly
			*/

			sendto(clientsocket, (char *) &ACK_packet, PACKET_SIZE, 
				0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
			printf("Sending packet %i", ACK_packet.seq_num);
            	if (packetType == PACKETSEND) {
				printf("\n");
			}
			if (packetType == RETRANS) {
				printf("Retransmission\n");
			}
			//if (next_packet > total_numPKT) {
			//	printf("Got the last packet\n");
			//	break;
			//}
		}
	} // End of receiving file packets while loop

	printf("Here finished.\n");




	/*
		Write the received packets into file
	*/
	//char fileContent[file_size + 1];
	char* fileContent;
	fileContent = (char *) calloc(file_size + 1, sizeof(char));
	//memset(fileContent, 0, file_size + 1);	

	printf("File space ready - time to copy\n");

	int i;
	for (i = 0; i < total_numPKT; i++) {
		strcat(fileContent, file_packets[i].buffer);
	}

	printf("FILE COPY DONE\n");

	fileContent[file_size] = '\0';

/*	FILE* f = fopen("test.txt", "wb");
	if (f == NULL) {
		Merror("Merror with opening file");
	}

	fwrite(fileContent, sizeof(char), file_size, f);

	fclose(f);*/


	close(clientsocket);

	return 0;
}


