#ifndef L3_PROTOCOLS_H
#define L3_PROTOCOLS_H

#include <cstdint>
#include <vector>
#include <chrono>

using namespace std;

#define ICMP_ECHO_REQUEST 	0x08
#define ICMP_ECHO_REPLY   	0x00

#define ARP_REQUEST			0x0001
#define ARP_REPLY			0x0002

struct ICMP {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t	sequence_number;
	std::vector<uint8_t> payload;
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
	std::vector<uint8_t> mac;
};

class ARP_cache {
	public:
		std::vector<uint8_t> get_mac(uint32_t);
		void add_entry(uint32_t, std::vector<uint8_t>);
		void clear();
	private:
		std::vector<ARP_entry> cache;
};

void fill_ICMP_payload(ICMP);

ARP generate_ARP(vector<uint8_t> arp_u8);

ICMP generate_ICMP(vector<uint8_t> icmp_u8);


#endif