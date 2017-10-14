#ifndef WINDOW_H_
#define WINDOW_H_

#include <time.h>
#include <stdlib.h>
#include "Packet.h"
#include <vector>
using namespace std;

enum WindowElementStatus {
	NOT_SENT, SENT, resent, acked
};

class WindowElement {
	bool hasStarted;
	struct timespec start;
	WindowElementStatus status;
	Packet* packet;
	unsigned int fr_count;
public:
	WindowElement(Packet* p);
	void setStartTimer();
	void setStatus(WindowElementStatus s);
	void incrementFrCountGivenAckPacket(Packet* ack);
	unsigned int getFrCount();
	int getTimeElapsedMs();
	Packet* getPacket();
	WindowElementStatus getStatus();
};

class Window {
	unsigned int window_size;
	unsigned int ssthresh;
	int packet_capacity; //max num of packets in window
	vector<Packet*> pending_packets;
	vector<WindowElement*> elements;
public:
	Window(unsigned int window_size_bytes);
	void appendAllPackets(vector<Packet*> all_packets);
	bool isEmpty();
	bool hasUnsentPacket();
	int getElementCount();
	unsigned int getWindowSize();
	unsigned int getSsthresh();
	void fillWindow();
	void removeWindowElementsByPacketType(PacketType type);
	bool removeAckedWindowElementBySeqnumAndWrapCount(unsigned int seqnum,
			unsigned int wrap_count);
	void appendAndStartPacketTimer(Packet* p);
	void printWindow();
	void clear();
	void resetWindowSize(unsigned int new_size);
	void resetSsthresh(unsigned int new_thresh);
	WindowElement* doFrCountAndGetWindowElementForRetransmission(Packet* ack);
	WindowElement* getNextWindowElementToSend();
	WindowElement* getTimedOutWindowElement();
};

#endif /* PACKET_H_ */
