#ifndef L3_PROTOCOLS_H
#define L3_PROTOCOLS_H

#include <cstdint>
#include <vector>

using namespace std;

struct ICMP {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint8_t	sequence_number;
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

ARP generate_ARP(vector<uint8_t> arp_u8);


#endif