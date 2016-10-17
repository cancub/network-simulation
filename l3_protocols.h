#ifndef L3_PROTOCOLS_H
#define L3_PROTOCOLS_H

#include <cstdint>
#include <vector>

using namespace std;

class ICMP {
	public:
		ICMP();
		ICMP(uint8_t);
		ICMP(uint8_t,uint8_t);
		ICMP(const ICMP & obj);
		void increment_sequence_number();
	private:
		uint8_t type;
		uint8_t code;
		uint16_t checksum;
		uint8_t	sequence_number;
		std::vector<uint8_t> payload;
};

class ARP_entry {
	public:
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


#endif