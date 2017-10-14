#ifndef PACKET_H_
#define PACKET_H_

#include <string>
#include "settings.h"
using namespace std;

#define PACKET_SIZE 1024
#define PACKET_DATA_SIZE (PACKET_SIZE - sizeof(PacketType) - sizeof(unsigned long) - 3*sizeof(unsigned int))

extern const char* FILE_NOT_FOUND_MSG;

enum PacketType {
	DATA, ACK, SYN, SYN_ACK, FIN, FIN_ACK, MSG, REQ
};


class Packet {
	PacketType type;
	char buffer[PACKET_DATA_SIZE];
	unsigned long filesize;
public:
	unsigned int seqnum;
	unsigned int wrap_count;
	unsigned int buffer_size;

	Packet(PacketType type, unsigned int seqnum, unsigned long filesize,
			unsigned int wrap_count);
	PacketType getPacketType();
	unsigned int getSeqnum();
	unsigned int getWrapCount();
	unsigned int getBufferSize();
	unsigned long getFilesize();
	char* getBuffer();
	char* getPacketTypeToPrint();
	void setBuffer(char* buffer, int size);
	void copyPacketBuffer(Packet* from);
	bool isTheSamePacket(Packet* p);
}
;
#endif /* PACKET_H_ */
