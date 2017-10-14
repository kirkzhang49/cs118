#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <signal.h> /* signal name macros, and the kill() prototype */
#include <string>
#include <cstring>

#include "settings.h"
#include "utils.h"
#include "Packet.h"
#include "Window.h"

using namespace std;

int sockfd;
struct sockaddr_in cli_addr;
socklen_t cli_len = sizeof(cli_addr);
unsigned int current_seqnum = 0;

void init(int argc, char *argv[]) {
	int portno;
	struct sockaddr_in serv_addr;
	if (argc < 2) {
		printErrorAndExit("no port provided");
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printErrorAndExit("cannot open socket");
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	printf("\n\n[INFO] Congestion Control Enabled?: %s\n",
			CC_ENABLED ? "true" : "false");
	printf("[INFO] Port number set to: %d\n", portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printErrorAndExit("cannot bind");
	}
	printf("[INFO] Waiting for request(s)...\n");
}

unsigned int getNextSeqnum() {
	if (current_seqnum + PACKET_SIZE > MAX_SEQNUM) {
		current_seqnum = 0;
		return current_seqnum;
	}
	current_seqnum += 1024;
	return current_seqnum;
}

vector<Packet*> splitFileToPackets(string filename) {
	vector<Packet*> all_packets;
	char* file_content;
	char *filepath = new char[filename.length() + 1];
	strcpy(filepath, filename.c_str());
	file_content = readFileToCharArray(filepath);
	unsigned long file_length = getFileLength(filepath);

	int num_of_packets;
	int fraction = file_length % PACKET_DATA_SIZE;
	bool has_fraction = (fraction != 0);

	num_of_packets =
			has_fraction ?
					(file_length / PACKET_DATA_SIZE + 1) :
					(file_length / PACKET_DATA_SIZE);
	int wrap_count = 0;

	for (int i = 0; i < num_of_packets; i++) {
		unsigned int seq = getNextSeqnum();
		if (seq == 0) {
			wrap_count++;
		}
		Packet* cur = new Packet(DATA, seq, file_length, wrap_count);
		if (i == num_of_packets - 1 && has_fraction) {
			cur->setBuffer(file_content + i * PACKET_DATA_SIZE, fraction);
		} else {
			cur->setBuffer(file_content + i * PACKET_DATA_SIZE,
			PACKET_DATA_SIZE);

		}
		all_packets.push_back(cur);
	}
	return all_packets;
}

void sendPacket(Window* window, Packet* packet, bool retransmission) {

	if (CC_ENABLED) {
		switch (packet->getPacketType()) {
		case SYN_ACK:
			printf("Sending packet %d %d %d SYN\n", packet->getSeqnum(),
					window->getWindowSize(), window->getSsthresh());
			break;
		case MSG:
			printf("Sending packet MSG\n");
			break;
		case DATA:
			if (retransmission) {
				printf("Sending packet %d %d %d Retransmission\n",
						packet->getSeqnum(), window->getWindowSize(),
						window->getSsthresh());
			} else {
				printf("Sending packet %d %d %d\n", packet->getSeqnum(),
						window->getWindowSize(), window->getSsthresh());
			}
			break;
		case FIN:
			printf("Sending packet %d %d %d FIN\n", packet->getSeqnum(),
					window->getWindowSize(), window->getSsthresh());
			break;
		case FIN_ACK:
			printf("Sending packet %d %d %d FIN-ACK\n", packet->getSeqnum(),
					window->getWindowSize(), window->getSsthresh());
			break;
		default:
			printf("Sending packet %d %d %d\n", packet->getSeqnum(),
					window->getWindowSize(), window->getSsthresh());
			break;
		}
	} else {
		switch (packet->getPacketType()) {
		case SYN_ACK:
			printf("Sending packet %d %d SYN\n", packet->getSeqnum(),
					window->getWindowSize());
			break;
		case MSG:
			printf("Sending packet MSG\n");
			break;
		case DATA:
			if (retransmission) {
				printf("Sending packet %d %d Retransmission\n",
						packet->getSeqnum(), window->getWindowSize());
			} else {
				printf("Sending packet %d %d\n", packet->getSeqnum(),
						window->getWindowSize());
			}
			break;
		case FIN:
			printf("Sending packet %d %d FIN\n", packet->getSeqnum(),
					window->getWindowSize());
			break;
		case FIN_ACK:
			printf("Sending packet %d %d FIN-ACK\n", packet->getSeqnum(),
					window->getWindowSize());
			break;
		default:
			printf("Sending packet %d %d\n", packet->getSeqnum(),
					window->getWindowSize());
			break;
		}
	}

	sendto(sockfd, packet, sizeof(char) * PACKET_SIZE, 0,
			(struct sockaddr*) &cli_addr, sizeof(cli_addr));
}

int main(int argc, char *argv[]) {
	Window* control_window = new Window(CONTROL_WINDOW_SIZE_BYTES);
	init(argc, argv);
	char buffer[PACKET_SIZE];
	resetBuffer(buffer);
	bool done = false;
	bool transmission_started = false;
	Packet* MSG_packet = new Packet(MSG, 0, 0, 0);
	MSG_packet->setBuffer((char*) FILE_NOT_FOUND_MSG,
			strlen(FILE_NOT_FOUND_MSG));
	unsigned int initial_window_size =
			CC_ENABLED ? PACKET_SIZE : DEFAULT_WINDOW_SIZE_BYTES;
	Window* window = new Window(initial_window_size);
	unsigned int sent_fin_count = 0;

	while (1) {
		if (!control_window->isEmpty()) {
			WindowElement* temp = control_window->getTimedOutWindowElement();
			if (temp != NULL) {
				sendPacket(window, temp->getPacket(), true);
				temp->setStartTimer();
				if (temp->getPacket()->getPacketType() == FIN) {
					sent_fin_count++;
				}
				if (sent_fin_count > 2) {
					done = true;
				}
			}
//			control_window->printWindow();
		}
		if (done) {
			printf("[INFO] Closing socket...\n");
			close(sockfd);
			printf("[INFO] File transmission complete.\n");
			done = false;
			init(argc, argv);
			resetBuffer(buffer);
			transmission_started = false;
			current_seqnum = 0;
			sent_fin_count = 0;
			control_window->clear();
			window->clear();
			window = new Window(initial_window_size);
			printf("[INFO] Continue to wait for further request(s)...\n");
//			printf("[DEBUG] Control window is now empty: %s\n",
//					control_window->isEmpty() ? "true" : "false");
		}
		if (recvfrom(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT,
				(struct sockaddr*) &cli_addr, &cli_len) != -1) {
			Packet* received_packet = (Packet*) buffer;
			switch (received_packet->getPacketType()) {
			case SYN: {
				printf("Receiving packet SYN\n");
				Packet* SYNACK_packet = new Packet(SYN_ACK, getNextSeqnum(), 0,
						0);
				sendPacket(window, SYNACK_packet, false);
				control_window->appendAndStartPacketTimer(SYNACK_packet);
				break;
			}
			case FIN: {
				control_window->removeWindowElementsByPacketType(MSG);
				printf("Receiving packet FIN\n");
				Packet* FIN_ACK_packet = new Packet(FIN_ACK, getNextSeqnum(), 0,
						0);
				sendPacket(window, FIN_ACK_packet, false);
//				control_window->appendAndStartPacketTimer(FIN_ACK_packet);
				Packet* FIN_packet = new Packet(FIN, getNextSeqnum(), 0, 0);
				sendPacket(window, FIN_packet, false);

				control_window->appendAndStartPacketTimer(FIN_packet);
				break;
			}
			case REQ: {
				unsigned int ca_count = 0;
				WindowElement* sent_for_fast_retrans = NULL;
				//only serve 1 request at a time
				if (transmission_started) {
					break;
				} else {
					transmission_started = true;
				}
				control_window->removeWindowElementsByPacketType(SYN_ACK);
				string filename = getStringFromBuffer(
						received_packet->getBuffer());
				printf("[INFO] File requested by client: %s\n",
						filename.c_str());

				if (!fileExists(filename)) {
					sendPacket(window, MSG_packet, false);
					control_window->appendAndStartPacketTimer(MSG_packet);
					done = true;
					printf("[INFO] %s\n", FILE_NOT_FOUND_MSG);
					printf(
							"[INFO] Continue to wait for further request(s)...\n");
				} else {
					printf("[INFO] Requested file exists\n");
					vector<Packet*> all_packets = splitFileToPackets(filename);
					window->appendAllPackets(all_packets);
					window->fillWindow();
//					printf("[DEBUG] window is empty? %s\n",
//							window->isEmpty() ? "true" : "false");

					while (!window->isEmpty() || window->hasUnsentPacket()) {
						WindowElement* nextWindowElementToSend =
								window->getNextWindowElementToSend();
						if (nextWindowElementToSend != NULL) {
							Packet* nextPacketToSend =
									nextWindowElementToSend->getPacket();
							sendPacket(window, nextPacketToSend, false);
							nextWindowElementToSend->setStartTimer();
							nextWindowElementToSend->setStatus(SENT);
						}

						if (recvfrom(sockfd, buffer, sizeof(buffer),
						MSG_DONTWAIT, (struct sockaddr*) &cli_addr, &cli_len)
								!= -1) {
							Packet* potential_ACK_packet = (Packet*) buffer;
							if (potential_ACK_packet->getPacketType() == ACK
									&& window->removeAckedWindowElementBySeqnumAndWrapCount(
											potential_ACK_packet->getSeqnum(),
											potential_ACK_packet->getWrapCount())) {
								printf("Receiving packet %d\n",
										potential_ACK_packet->getSeqnum());
								if (CC_ENABLED) {

									// fast recovery - after recovering missing packet
									if (sent_for_fast_retrans != NULL) {
										if (sent_for_fast_retrans->getPacket()->isTheSamePacket(
												potential_ACK_packet)) {
											window->resetWindowSize(
													window->getSsthresh());
											sent_for_fast_retrans = NULL;
//											printf(
//													"[CC-DEBUG] *** in fast recovery, after\n");
										}
									}
									//slow start
									if (window->getWindowSize()
											<= window->getSsthresh()) {
										window->resetWindowSize(
												window->getWindowSize() + PACKET_SIZE);

//										printf(
//												"[CC-DEBUG] *** in slow start\n");
									}

									//congestion avoidance
									else if (window->getWindowSize()
											> window->getSsthresh()) {
										ca_count++;
										if (ca_count
												== window->getWindowSize()
														/ PACKET_SIZE) {
											window->resetWindowSize(
													window->getWindowSize() + PACKET_SIZE);
											ca_count = 0;
//											printf("[CC-DEBUG] *** in CA\n");
										}
									}
									WindowElement* fast_retrans_window_element =
											window->doFrCountAndGetWindowElementForRetransmission(
													potential_ACK_packet);

									//fast retranmission
									if (fast_retrans_window_element != NULL) {
										if (fast_retrans_window_element->getFrCount()
												== 3) {
											fast_retrans_window_element->setStartTimer();
											window->resetSsthresh(
													getMax(
															window->getWindowSize()
																	/ 2,
															2 * PACKET_SIZE));
											window->resetWindowSize(
													window->getSsthresh()+3*PACKET_SIZE);
											sendPacket(window,
													fast_retrans_window_element->getPacket(),
													true);
											sent_for_fast_retrans =
													fast_retrans_window_element;
//											printf(
//													"[CC-DEBUG] *** in fast retrans\n");

										} else {
											// fast recovery - before recovering missing packet
											window->resetWindowSize(
													window->getWindowSize()+PACKET_SIZE);
//											printf(
//													"[CC-DEBUG] *** in fast recovery, before\n");
										}
									}

								}

							} else if (potential_ACK_packet->getPacketType()
									== FIN) {
								printf("Receiving packet FIN\n");
								Packet* FIN_ACK_packet = new Packet(FIN_ACK,
										getNextSeqnum(), 0, 0);
								sendPacket(window, FIN_ACK_packet, false);
								Packet* FIN_packet = new Packet(FIN,
										getNextSeqnum(), 0, 0);
								sendPacket(window, FIN_packet, false);
								control_window->appendAndStartPacketTimer(
										FIN_packet);
								break;
							}
						}

						WindowElement* timed_out_element =
								window->getTimedOutWindowElement();
						if (timed_out_element != NULL) {
//							printf("[DEBUG]*** Packet#%d timed out\n",
//									timed_out_element->getPacket()->getSeqnum());
							timed_out_element->setStartTimer();
							sendPacket(window, timed_out_element->getPacket(),
									true);
							window->resetSsthresh(
									getMax(window->getWindowSize() / 2,
											(unsigned int) 2 * PACKET_SIZE));
							window->resetWindowSize(PACKET_SIZE);
							ca_count = 0;
						}
						window->fillWindow();
//						printf("[DEBUG] Number of WindowElements: %d\n",
//								window->getElementCount());
					}
				}
				break;
			}
			case ACK: {
				//ignore ACK out of REQ loop
				break;
			}
			case FIN_ACK: {
				control_window->removeWindowElementsByPacketType(FIN);
				done = true;
				break;
			}
			default: {
				printf("[DEBUG] Type: %s\n", received_packet->getPacketTypeToPrint());
//				printErrorAndExit("Invalid packet type.");
				break;
			}
			}
		}
	} /* end of while */
	return 0;
}

