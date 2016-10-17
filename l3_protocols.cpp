#include "l3_protocols.h"
#include "addressing.h"
#include <iostream>
#include <cstdint>

ICMP::ICMP() {}

ICMP::ICMP(uint8_t packet_type){
	type = packet_type;
	sequence_number = 0;
}

ICMP::ICMP(uint8_t packet_type,uint8_t sn){
	type = packet_type;
	sequence_number = sn;
}

ICMP::ICMP(const ICMP & obj){
	sequence_number = obj.sequence_number;
	type = obj.type;
}

void ICMP::increment_sequence_number() {
	sequence_number++;
}


std::vector<uint8_t> ARP_cache::get_mac(uint32_t ip) {

	std::vector<uint8_t> result = create_broadcast_mac();


	for (std::vector<ARP_entry>::iterator it = cache.begin(); it != cache.end(); ++it) {
		if (compare_ips(ip, it->ip)) {
			result = it->mac;
			break;
		}
	}

	return result;
}

void ARP_cache::add_entry(uint32_t ip, std::vector<uint8_t> mac) {
	ARP_entry new_entry;
	new_entry.ip = ip;
	new_entry.mac = mac;
	cache.push_back(new_entry);
}

void ARP_cache::clear() {
	cache.clear();
}


// int main() {
// 	ARP_cache my_cache;
// 	std::vector<uint8_t> mac;
// 	std::vector<uint8_t> rand_mac = create_random_mac();
// 	uint32_t ip = create_random_ip();

// 	mac = my_cache.get_mac(ip);

// 	if (is_broadcast(mac)) {
// 		std::cout << "ip not found" << std::endl;
// 		my_cache.add_entry(ip,rand_mac);
// 	}

// 	mac = my_cache.get_mac(ip);

// 	if (is_broadcast(mac)) {
// 		std::cout << "ip not found" << std::endl;
// 	} else {
// 		std::cout << "ip: " << ip_to_string(ip) << ", mac: " << mac_to_string(mac) << std::endl;
// 	}


// 	return 0;
// }