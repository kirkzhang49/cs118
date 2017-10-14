#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <cstring>
#include <algorithm>
#include <time.h>

#include "settings.h"
#include "utils.h"
#include "Packet.h"
#include "Window.h"
using namespace std;

int sockfd;
struct sockaddr_in serv_addr;
socklen_t serv_len = sizeof(serv_addr);

bool comparator(Packet * a, Packet * b) {
	if (a->wrap_count != b->wrap_count) {
		return a->wrap_count < b->wrap_count;
	}
	return a->seqnum < b->seqnum;
}

void init(int argc, char *argv[]) {
	int portno;
	struct hostent *server; //contains tons of information, including the server's IP address

	if (argc < 4) {
		printErrorAndExit("not enough input arguments");
	}

	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a new socket
	if (sockfd < 0) {
		printErrorAndExit("cannot open socket");
	}

	server = gethostbyname(argv[1]); //takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
	if (server == NULL) {
		printErrorAndExit("no such host");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //initialize server's address
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
}

void combinePacketsToFile(vector<Packet*> all_packets, char* filename) {
	sort(all_packets.begin(), all_packets.end(), comparator);
	long file_length = all_packets.at(0)->getFilesize();
	char * output;
	output = (char *) calloc(file_length, sizeof(char));
	int offset = 0;
	for (int i = 0; i < (int) all_packets.size(); i++) {

		memcpy(output + offset, all_packets.at(i)->getBuffer(),
				all_packets.at(i)->getBufferSize());
		offset += all_packets.at(i)->getBufferSize();
	}

	writeCharArrayToFile(output, filename, file_length);
}

void sendPacket(Packet* packet, bool retransmission) {
	switch (packet->getPacketType()) {
	case SYN:
		if (retransmission) {
			printf("Sending packet SYN Retransmission\n");

		} else {
			printf("Sending packet SYN\n");
		}
		break;
	case REQ:
		printf("Sending packet ACK/REQ\n");
		break;
	case ACK:
		if (retransmission) {
			printf("Sending packet %d Retransmission\n", packet->getSeqnum());
		} else {
			printf("Sending packet %d\n", packet->getSeqnum());
		}
		break;
	case FIN_ACK:
		if (retransmission) {
			printf("Sending packet %d FIN-ACK Retransmission\n",
					packet->getSeqnum());
			break;
		} else {
			printf("Sending packet %d FIN-ACK\n", packet->getSeqnum());
		}
		break;
	case FIN:
		printf("Sending packet FIN\n");
		break;
	default:
		printf("Sending packet %d\n", packet->getSeqnum());
		break;
	}
	sendto(sockfd, packet, PACKET_SIZE * sizeof(char), 0,
			(struct sockaddr*) &serv_addr, sizeof(serv_addr));
}

bool isDuplicatePacket(vector<Packet*> all, Packet* p) {
	for (Packet* cur : all) {
		if (cur->getSeqnum() == p->getSeqnum()
				&& cur->getWrapCount() == p->getWrapCount()) {
			return true;
		}
	}
	return false;
}

int main(int argc, char *argv[]) {
	bool received_fin = false;
	bool trans_finished = false;
	init(argc, argv);
	char* filename = argv[3];
	char buffer[PACKET_SIZE];
	resetBuffer(buffer);
	printf("[INFO] Requesting file: %s\n", filename);

	Window* control_window = new Window(CONTROL_WINDOW_SIZE_BYTES);
	Packet* SYN_packet = new Packet(SYN, 0, 0, 0);
	Packet * FIN_ACK_packet;
	sendPacket(SYN_packet, false);

	control_window->appendAndStartPacketTimer(SYN_packet);
//	unsigned int lastSeqNum = 0;

	while (1) {

		if (!control_window->isEmpty()) {
			WindowElement* temp = control_window->getTimedOutWindowElement();
			if (temp != NULL) {
				sendPacket(temp->getPacket(), true);
				temp->setStartTimer();
			}
		}
		//timed wait
		if (received_fin) {
			printf("[INFO] Entering timed wait...\n");
			struct timespec start;
			struct timespec end;
			unsigned int delta_ms = 0;
			clock_gettime(CLOCK_MONOTONIC_RAW, &start);
			while (delta_ms < 2 * TIMEOUT_MS) {
				clock_gettime(CLOCK_MONOTONIC_RAW, &end);
				delta_ms = (end.tv_sec - start.tv_sec) * 1000
						+ (end.tv_nsec - start.tv_nsec) / 1000000;

				if (recvfrom(sockfd, buffer, sizeof(buffer),
				MSG_DONTWAIT, (struct sockaddr*) &serv_addr, &serv_len) != -1) {

					Packet* received_packet = (Packet*) buffer;
					if (received_packet->getPacketType() == FIN) {
						printf("Receiving packet FIN\n");
						Packet* FIN_ACK_packet = new Packet(FIN_ACK,
								received_packet->getSeqnum(), 0, 0);
						sendPacket(FIN_ACK_packet, false);
					}
				}
			}
//			printf("[DEBUG]*** Control window is now empty: %s\n",
//					control_window->isEmpty() ? "true" : "false");
			close(sockfd);
			printf("[INFO] Closing socket...\n");
			break;
		}
		if (recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT,
				(struct sockaddr*) &serv_addr, &serv_len) != -1) {
			Packet* received_packet = (Packet*) buffer;
			switch (received_packet->getPacketType()) {
			case SYN_ACK: {
				control_window->removeWindowElementsByPacketType(SYN);
				printf("Receiving packet SYN-ACK\n");
				Packet* REQ_packet = new Packet(REQ,
						received_packet->getSeqnum(), 0, 0);
				REQ_packet->setBuffer(filename, strlen(filename));
				sendPacket(REQ_packet, false);
				control_window->appendAndStartPacketTimer(REQ_packet);
				break;
			}
			case MSG: {
				control_window->removeWindowElementsByPacketType(REQ);
				string message = getStringFromBuffer(
						received_packet->getBuffer());
				if (message.compare(FILE_NOT_FOUND_MSG) == 0) {
					trans_finished = true;
					Packet* FIN_packet = new Packet(FIN, 0, 0, 0);
					sendPacket(FIN_packet, false);
					control_window->appendAndStartPacketTimer(FIN_packet);
					break;
				} else {
					close(sockfd);
					printf("[DEBUG] Invalid MSG type: %s\n", message.c_str());
//					printErrorAndExit("Invalid MSG type");
				}
				break;
			}
			case DATA: {
				if (trans_finished) {
					Packet* ACK_packet = new Packet(ACK,
							received_packet->getSeqnum(), 0,
							received_packet->getWrapCount());
					sendPacket(ACK_packet, true);
					break;
				}
				control_window->removeWindowElementsByPacketType(REQ);
				Packet* first_received_packet = (Packet*) buffer;
				printf("Receiving packet %d\n",
						first_received_packet->getSeqnum());
				Packet* first_ACK_packet = new Packet(ACK,
						first_received_packet->getSeqnum(), 0,
						first_received_packet->getWrapCount());
				sendPacket(first_ACK_packet, false);

				long file_size = first_received_packet->getFilesize();
				Packet* packet_copy = new Packet(DATA,
						first_received_packet->getSeqnum(),
						first_received_packet->getFilesize(),
						first_received_packet->getWrapCount());
				packet_copy->copyPacketBuffer(first_received_packet);
				vector<Packet*> all_packets;
				all_packets.push_back(packet_copy);
				int count = 1;
				int expected_packet_num =
						(file_size % PACKET_DATA_SIZE == 0) ?
								file_size / PACKET_DATA_SIZE :
								file_size / PACKET_DATA_SIZE + 1;
				resetBuffer(buffer);

				while (count < expected_packet_num) {
					if (recvfrom(sockfd, buffer, sizeof(buffer), 0,
							(struct sockaddr*) &serv_addr, &serv_len) != -1) {
						Packet* subsequent_packet = (Packet*) buffer;
						printf("Receiving packet %d\n",
								subsequent_packet->getSeqnum());
						Packet* subsequent_packet_copy = new Packet(DATA,
								subsequent_packet->getSeqnum(),
								subsequent_packet->getFilesize(),
								subsequent_packet->getWrapCount());
						subsequent_packet_copy->copyPacketBuffer(
								subsequent_packet);
						Packet* ACK_packet = new Packet(ACK,
								subsequent_packet->getSeqnum(), 0,
								subsequent_packet->getWrapCount());
						if (!isDuplicatePacket(all_packets,
								subsequent_packet_copy)) {

							sendPacket(ACK_packet, false);
							all_packets.push_back(subsequent_packet_copy);
							count++;
//							printf("[DEBUG] received (%d/%d) packets\n", count,
//									expected_packet_num);

						} else {
							sendPacket(ACK_packet, true);
						}
						resetBuffer(buffer);
					}
				}
				combinePacketsToFile(all_packets, filename);
				trans_finished = true;
				Packet* FIN_packet = new Packet(FIN, 0, 0, 0);
				sendPacket(FIN_packet, false);
				control_window->appendAndStartPacketTimer(FIN_packet);
				break;
			}
			case FIN: {
				control_window->removeWindowElementsByPacketType(FIN);
				printf("Receiving packet FIN\n");
				FIN_ACK_packet = new Packet(FIN_ACK,
						received_packet->getSeqnum(), 0, 0);
				sendPacket(FIN_ACK_packet, false);
				received_fin = true;
				break;
			}
			case FIN_ACK: {
				printf("Receiving packet FIN-ACK\n");
				break;
			}
			default: {
				printf("[DEBUG] Type: %s\n",
						received_packet->getPacketTypeToPrint());
//				printErrorAndExit("Invalid packet type.");
				break;
			}
			}
		}
	}
	return 0;
}

