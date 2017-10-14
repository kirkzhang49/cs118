#include "Packet.h"
#include <cstring>

using namespace std;

const char* FILE_NOT_FOUND_MSG = "Requested file does not exist";

Packet::Packet(PacketType type, unsigned int seqnum, unsigned long filesize,
		unsigned int wrap_count) {
	this->type = type;
	this->seqnum = seqnum;
	this->filesize = filesize;
	this->wrap_count = wrap_count;
	this->buffer_size = 0;
}
PacketType Packet::getPacketType() {
	return this->type;
}
void Packet::setBuffer(char* buffer, int size) {
	memcpy(this->buffer, buffer, size);
	this->buffer_size = size;
}
char* Packet::getBuffer() {
	return this->buffer;
}

unsigned int Packet::getBufferSize() {
	return buffer_size;
}

unsigned int Packet::getSeqnum() {
	return seqnum;
}

unsigned int Packet::getWrapCount() {
	return wrap_count;
}

unsigned long Packet::getFilesize() {
	return filesize;
}
void Packet::copyPacketBuffer(Packet* from) {
	char* buffer = from->getBuffer();
	int buffer_size = from->getBufferSize();
	this->setBuffer(buffer, buffer_size);
}

char* Packet::getPacketTypeToPrint() {
	char* ret;
	switch (type) {
	case DATA:
		ret = (char *) "DATA";
		break;
	case ACK:
		ret = (char *) "ACK";
		break;
	case SYN:
		ret = (char *) "SYN";
		break;
	case SYN_ACK:
		ret = (char *) "SYN_ACK";
		break;
	case FIN:
		ret = (char *) "FIN";
		break;
	case FIN_ACK:
		ret = (char *) "FIN_ACK";
		break;
	case MSG:
		ret = (char *) "MSG";
		break;
	case REQ:
		ret = (char *) "REQ";
		break;
	default:
		ret = (char *) "DATA";
		break;
	}
	return ret;
}

bool Packet::isTheSamePacket(Packet* p) {
	return seqnum == p->getSeqnum() && wrap_count == p->getWrapCount();
}

