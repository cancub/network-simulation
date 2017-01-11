#ifndef L4_PROTOCOLS_H
#define L4_PROTOCOLS_H

#include <vector>
#include <list>
#include <cstdint>
// #include "pdu.h"
// #include "data_links.h"

using namespace std;


#define TCP_SYN 	0x0002
#define TCP_ACK 	0x0010
#define TCP_FIN 	0x0001
#define TCP_SYNACK 	(TCP_ACK | TCP_SYN)
#define TCP_FINACK 	(TCP_ACK | TCP_FIN)

#define TCP_INITIAL_SN 0

#define TCP_HEADER_SIZE 18
#define UDP_HEADER_SIZE 8

//  layer 4 types

// 	UDP

struct UDP {
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
	vector<uint8_t> payload;
};

UDP generate_UDP(vector<uint8_t> udp_u8);

vector<UDP> * file_to_UDP_segments(const char * filename, uint16_t src_port, uint16_t dest_port, int maximum_bytes);

// TCP

struct TCP {
	uint16_t source_port;
	uint16_t destination_port;
	uint32_t sequence_number;
	uint32_t ACK_number;
	uint16_t details; // includes header length, SYN, FIN, ACK
	uint16_t receive_window;
	uint16_t checksum;
	vector<uint8_t> payload;
};

// class TCPSegmentList {
// public:
// 	TCPSegmentList();
// 	~TCPSegmentList();
// 	void add(TCP);
// 	void remove(TCP);
// 	void pop_front() {segment_list.pop_front();}
// 	TCP front() {return segment_list.front();}
// 	size_t size() {return segment_list.size();}
// private:
// 	list<TCP> segment_list;
// }

TCP generate_TCP(vector<uint8_t> tcp_u8);

vector<TCP> * file_to_TCP_segments(const char *, uint16_t src_port, uint16_t dest_port, int maximum_bytes);

// MPDU* TCP_to_MPDU();

#endif