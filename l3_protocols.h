#ifndef L3_PROTOCOLS_H
#define L3_PROTOCOLS_H

#include <cstdint>
#include <vector>
#include <chrono>
#include <stdlib.h>
#include "l4_protocols.h"

using namespace std;

#define ICMP_ECHO_REQUEST 	0x08
#define ICMP_ECHO_REPLY   	0x00

#define ARP_REQUEST			0x0001
#define ARP_REPLY			0x0002

#define IP_PROTOCOL_ICMP    0x01
#define IP_PROTOCOL_TCP     0x06
#define IP_PROTOCOL_UDP     0x11

#define IP_HEADER_SIZE      12
#define ICMP_HEADER_SIZE    8

struct ICMP {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identifier;
	uint16_t sequence_number;
	vector<uint8_t> payload;
};

struct ARP {
	uint16_t opcode;
	vector<uint8_t> sender_mac;
	uint32_t sender_ip;
	vector<uint8_t> target_mac;
	uint32_t target_ip;
};

struct ARP_entry {
	uint32_t ip;
	vector<uint8_t> mac;
};

class ARP_cache {
	public:
		vector<uint8_t> get_mac(uint32_t);
		void add_entry(uint32_t, vector<uint8_t>);
		void clear();
	private:
		vector<ARP_entry> cache;
};

class IP { 
    public:
        IP();
        IP(vector<uint8_t>);
        void encap_SDU(ICMP);
        void encap_SDU(TCP);
        void encap_SDU(UDP);
        void set_source_ip(uint32_t);
        void set_destination_ip(uint32_t);
        uint32_t get_source_ip();
        uint32_t get_destination_ip();
        uint8_t get_protocol();
        uint16_t get_SDU_length();
        uint8_t get_header_length();
        uint16_t get_total_length();
        vector<uint8_t> get_SDU();
        ~IP();
    private:     
        uint8_t header_length;
        uint16_t total_length;
        uint8_t protocol;
        uint32_t source_ip;
        uint32_t destination_ip;

        vector<uint8_t> SDU;
};


ARP generate_ARP(vector<uint8_t> arp_u8);

uint8_t get_ARP_opcode(vector<uint8_t> * arp_u8_ptr);

ICMP generate_ICMP(vector<uint8_t> icmp_u8);

uint8_t get_ping_type(vector<uint8_t> * ip_u8_ptr);

IP generate_IP(vector<uint8_t>);

uint8_t get_IP_protocol(vector<uint8_t> * ip_u8_ptr);

#endif