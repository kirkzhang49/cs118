#include "Window.h"
#include "utils.h"
#include "settings.h"

#include <time.h>
#include <algorithm>

using namespace std;

WindowElement::WindowElement(Packet* p) {
	packet = p;
	status = NOT_SENT;
	hasStarted = false;
	fr_count = 0;
}

void WindowElement::incrementFrCountGivenAckPacket(Packet* ack) {
	if (ack->getWrapCount() > packet->getWrapCount()) {
		fr_count++;
	} else if (ack->getWrapCount() == packet->getWrapCount()
			&& ack->getSeqnum() > packet->getSeqnum()) {
		fr_count++;
	}
}

unsigned int WindowElement::getFrCount() {
	return fr_count;
}

void WindowElement::setStartTimer() {
	hasStarted = true;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
}

int WindowElement::getTimeElapsedMs() {
	if (hasStarted) {
		struct timespec end;
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		int delta_ms = (end.tv_sec - start.tv_sec) * 1000
				+ (end.tv_nsec - start.tv_nsec) / 1000000;
		return (int) delta_ms;
	}
	return 0;
}

Packet* WindowElement::getPacket() {
	return packet;
}

WindowElementStatus WindowElement::getStatus() {
	return status;
}
void WindowElement::setStatus(WindowElementStatus s) {
	status = s;
}

Window::Window(unsigned int window_size_bytes) {
	this->window_size = window_size_bytes;
	this->ssthresh = INITIAL_ssthresh;
	this->packet_capacity = window_size_bytes / PACKET_SIZE;
//	this->pending_packets = NULL;
}

void Window::appendAllPackets(vector<Packet*> all_packets) {
	this->pending_packets = all_packets;
	reverse(pending_packets.begin(), pending_packets.end());
}

void Window::fillWindow() {
	while ((int) elements.size() < packet_capacity && !pending_packets.empty()) {
		Packet* p = pending_packets.back();
		pending_packets.pop_back();
		WindowElement* e = new WindowElement(p);
		elements.push_back(e);
	}
}

bool Window::isEmpty() {
	return (elements.size() == 0);
}

bool Window::hasUnsentPacket() {
	if (isEmpty()) {
		return false;
	}
	for (WindowElement* e : elements) {
		if (e->getStatus() == NOT_SENT) {
			return true;
		}
	}
	return false;
}

WindowElement* Window::getNextWindowElementToSend() {
	for (WindowElement* e : elements) {
		if (e->getStatus() == NOT_SENT) {
			return e;
		}
	}
	return NULL;
}

void Window::appendAndStartPacketTimer(Packet* p) {
	if (packet_capacity <= getElementCount()) {
		printf(
				"Appending to a filled window, window cap: %d, element count: %d\n",
				packet_capacity, getElementCount());
		exit(1);
	}
	for (WindowElement* e : elements) {
		if (e->getPacket()->getPacketType() == p->getPacketType()) {
			return;
		}
	}
	WindowElement* temp = new WindowElement(p);
	temp->setStartTimer();
	this->elements.push_back(temp);
}

void Window::removeWindowElementsByPacketType(PacketType type) {
	bool done = false;
	while (!done && elements.size() != 0) {
		for (int i = 0; i < (int) elements.size(); i++) {
			if (elements.at(i)->getPacket()->getPacketType() == type) {
				elements.erase(elements.begin() + i);
				break;
			}
			done = true;
		}
	}
}

bool Window::removeAckedWindowElementBySeqnumAndWrapCount(unsigned int seqnum,
		unsigned int wrap_count) {
	for (int i = 0; i < (int) elements.size(); i++) {
		Packet* cur = elements.at(i)->getPacket();
		if (cur->getSeqnum() == seqnum && cur->getWrapCount() == wrap_count) {
			elements.erase(elements.begin() + i);
			return true;
		}
	}
	return false;
}

int Window::getElementCount() {
	return elements.size();
}

void Window::clear() {
	elements.clear();
}

void Window::printWindow() {
	for (WindowElement* w : elements) {
		printf("[DEBUG]*** ");
		printf("[%s]-", w->getPacket()->getPacketTypeToPrint());
	}
	printf("|\n");
}

WindowElement* Window::getTimedOutWindowElement() {
	for (WindowElement* e : elements) {
		if (e->getTimeElapsedMs() > (int) TIMEOUT_MS) {
//			printf("[DEBUG]*** time elapsed: %d\n", e->getTimeElapsedMs());
			return e;
		}
	}
	return NULL;
}

void Window::resetWindowSize(unsigned int new_size) {
	this->window_size = new_size;
	packet_capacity = new_size / PACKET_SIZE;
	if (packet_capacity < getElementCount()) {
		for (int i = getElementCount() - 1; i >= 0; i--) {
			Packet* p = elements.at(i)->getPacket();
			pending_packets.push_back(p);
		}
		clear();
		fillWindow();
	}
}

unsigned int Window::getWindowSize() {
	return this->window_size;
}

void Window::resetSsthresh(unsigned int new_thresh) {
	this->ssthresh = new_thresh;
}

unsigned int Window::getSsthresh() {
	return this->ssthresh;
}

WindowElement* Window::doFrCountAndGetWindowElementForRetransmission(
		Packet* ack) {
	if (isEmpty()) {
		return NULL;
	}
	elements.at(0)->incrementFrCountGivenAckPacket(ack);
	return elements.at(0)->getFrCount() >= 3 ? elements.at(0) : NULL;
}
