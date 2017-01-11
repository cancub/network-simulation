#include "l3_protocols.h"
#include "l4_protocols.h"
#include "addressing.h"
#include <iostream>
#include <cstdint>
#include <chrono>

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


uint8_t get_ARP_opcode(vector<uint8_t> * arp_u8_ptr) {
    uint8_t result = arp_u8_ptr->at(1);
    // cout << result << endl;
    return result;
}





ICMP generate_ICMP(vector<uint8_t> icmp_u8) {

	ICMP icmp_result;
	// uint8_t type;
	icmp_result.type = icmp_u8[0];

	// uint8_t code;
	icmp_result.code = icmp_u8[1];

	// uint16_t checksum;
	icmp_result.checksum = (((uint16_t)(icmp_u8[2]) << 8) & 0xFF00) + (icmp_u8[3]);

	// uint16_t	identifier;
	icmp_result.identifier = (((uint16_t)(icmp_u8[4]) << 8) & 0xFF00) + (icmp_u8[5]);

	// uint16_t	sequence_number;
	icmp_result.sequence_number = (((uint16_t)(icmp_u8[6]) << 8) & 0xFF00) + (icmp_u8[7]);

	// std::vector<uint8_t> payload;
	icmp_result.payload.reserve(icmp_u8.size() - 8);
	for (int i = 8; i < icmp_u8.size(); i++) {
		icmp_result.payload.push_back(icmp_u8[i]);
	}
	
	return icmp_result;
}

uint8_t get_ping_type(vector<uint8_t> * ip_u8_ptr) {
    // we need to find the type, so we have to skip past the IP header
    return ip_u8_ptr->at(IP_HEADER_SIZE);
}











IP::IP(){
    header_length = 12; // header_length + total_length + protocol + source and dest IPs
}

// take a u8 version of the IP frame and create an IP object from it
IP::IP(std::vector<uint8_t> ip_u8) {

    int i = 0;
    // uint8_t header_length;
    header_length = ip_u8[0];

    // uint16_t total_length;
    total_length = ((uint16_t)(ip_u8[1]) << 8) + ip_u8[2];

    // uint8_t protocol;
    protocol = ip_u8[3];

    // uint32_t source_ip;
    source_ip = create_ip(ip_u8[4],ip_u8[5],ip_u8[6],ip_u8[7]);

    // uint32_t destination_ip;
    destination_ip = create_ip(ip_u8[8],ip_u8[9],ip_u8[10],ip_u8[11]);

    // std::vector<uint8_t> SDU;
    SDU.reserve(total_length - header_length);
    for (int i = 0; i < total_length - header_length; i++) {
        SDU.push_back(ip_u8[i+12]);
    }
}

IP::~IP() {}

void IP::encap_SDU(ICMP new_ICMP) {
    // set the type since we know it's ARP

    protocol = IP_PROTOCOL_ICMP; // ICMP

    // cout << "header length set to " << std::to_string(header_length) << endl;

    SDU.clear();
    SDU.reserve(8 + new_ICMP.payload.size());

    int j = 0;

    SDU.push_back(new_ICMP.type);
    SDU.push_back(new_ICMP.code);

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ICMP.checksum >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ICMP.identifier >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ICMP.sequence_number >> (i*8)) & 0xFF);
    }

    for (int i = 0; i < new_ICMP.payload.size(); i++) {
        SDU.push_back(new_ICMP.payload[i]);
    }

    total_length = header_length + SDU.size();

}




void IP::encap_SDU(TCP new_tcp) {
    // encap a TCP segment
    // NOTE: do this with std::copy
    protocol = IP_PROTOCOL_TCP;

    SDU.clear();
    SDU.reserve(TCP_HEADER_SIZE + new_tcp.payload.size());

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_tcp.source_port>> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_tcp.destination_port >> (i*8)) & 0xFF);
    }

    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_tcp.sequence_number >> (i*8)) & 0xFF);
    }

    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_tcp.ACK_number >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_tcp.details >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_tcp.receive_window >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_tcp.checksum >> (i*8)) & 0xFF);
    }

    for (int i = 0; i < new_tcp.payload.size(); i++) {
        SDU.push_back(new_tcp.payload[i]);
    }

    total_length = header_length + SDU.size();
}

void IP::encap_SDU(UDP new_udp) {
    // encap a UDP segment
    // NOTE: do this with std::copy
    protocol = IP_PROTOCOL_UDP;

    SDU.clear();
    SDU.reserve(UDP_HEADER_SIZE + new_udp.payload.size());// 18 bytes is the size of the TCP header at this moment

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_udp.source_port>> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_udp.destination_port >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_udp.length >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_udp.checksum >> (i*8)) & 0xFF);
    }

    for (int i = 0; i < new_udp.payload.size(); i++) {
        SDU.push_back(new_udp.payload[i]);
    }

    total_length = header_length + SDU.size();

}


void IP::set_source_ip(uint32_t input_source_ip) {source_ip = input_source_ip;}
void IP::set_destination_ip(uint32_t input_destination_ip) {destination_ip = input_destination_ip;}
uint8_t IP::get_header_length() {return header_length;}
uint16_t IP::get_total_length() {return total_length;}
uint32_t IP::get_source_ip() {return source_ip;}
uint32_t IP::get_destination_ip() {return destination_ip;}
uint8_t IP::get_protocol() {return protocol;}
uint16_t IP::get_SDU_length() {return SDU.size();}
std::vector<uint8_t> IP::get_SDU() {return SDU;}

IP generate_IP(std::vector<uint8_t> ip_u8) {

    IP to_return(ip_u8);

    return to_return;
}

uint8_t get_IP_protocol(vector<uint8_t> * ip_u8_ptr) {
    // the protocol is the 4th byte
    return ip_u8_ptr->at(3);
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