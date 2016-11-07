#include "l3_protocols.h"
#include "addressing.h"
#include <iostream>
#include <cstdint>

// #define DEBUG


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

#ifdef DEBUG
	for (int i = 0; i < cache.size(); i++) {
		std::cout << "IP: " << ip_to_string(cache[i].ip) << " <-> MAC: " << 
			mac_to_string(cache[i].mac)<< std::endl; 
	}
#endif

}

void ARP_cache::clear() {
	cache.clear();
}


ARP generate_ARP(vector<uint8_t> arp_u8) {
	ARP arp_result;
	arp_result.opcode = (((uint16_t)(arp_u8[0]) << 8) & 0xFF00) + 
							(((uint16_t)(arp_u8[1])) & 0x00FF);

	arp_result.sender_mac.reserve(6);
	for (int i = 2; i < 8; i++) {
		arp_result.sender_mac.push_back(arp_u8[i]);
	}

	arp_result.sender_ip = 0x00000000;
	for (int i = 8; i < 12; i++) {
		arp_result.sender_ip |= ((uint32_t)(arp_u8[i]) &0x000000FF ) << (8 * (11-i));
	}


	arp_result.target_mac.reserve(6);
	for (int i = 12; i < 18; i++) {
		arp_result.target_mac.push_back(arp_u8[i]);
	}

	arp_result.target_ip = 0x00000000;
	for (int i = 18; i < 22; i++) {
		arp_result.target_ip |= ((uint32_t)(arp_u8[i]) &0x000000FF ) << (8 * (21-i));
	}

	return arp_result;

}

ICMP generate_ICMP(vector<uint8_t> icmp_u8) {
	// for (int i = 0; i < icmp_u8.size(); i++){
	// 	cout << to_string(icmp_u8[i]) << " ";
	// }
	// cout << endl;

	ICMP icmp_result;
	// uint8_t type;
	icmp_result.type = icmp_u8[0];

	// uint8_t code;
	icmp_result.code = icmp_u8[1];

	// uint16_t checksum;
	// cout << to_string(icmp_u8[2]) << endl;
	// cout << to_string((uint16_t)(icmp_u8[2])) << endl;
	// cout << to_string((uint16_t)(icmp_u8[2]) << 8) << endl;
	// cout << to_string((((uint16_t)(icmp_u8[2]) << 8) & 0xFF00)) << endl;;
	icmp_result.checksum = (((uint16_t)(icmp_u8[2]) << 8) & 0xFF00) + (icmp_u8[3]);

	// uint8_t	sequence_number;
	icmp_result.sequence_number = (((uint16_t)(icmp_u8[4]) << 8) & 0xFF00) + (icmp_u8[5]);

	// std::vector<uint8_t> payload;
	icmp_result.payload.reserve(icmp_u8.size() - 6);
	for (int i = 6; i < icmp_u8.size(); i++) {
		icmp_result.payload.push_back(icmp_u8[i]);
	}
	
	return icmp_result;
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