#include <stdlib.h>
#include <vector>
#include <iostream>
#include "sim_tcpdump.h"
#include "addressing.h"
#include "pdu.h"
#include "l3_protocols.h"
#include "l4_protocols.h"
#include "networking_devices.h"

using namespace std;

string *  get_frame_details(MPDU* tx_frame) {
	vector<uint8_t> payload = tx_frame->get_SDU();
	string temp;
	temp = mac_to_string(tx_frame->get_source_mac()) + " " + mac_to_string(tx_frame->get_destination_mac());
	// get the type of the SDU
	switch(tx_frame->get_SDU_type()) {
		case MPDU_ARP_TYPE:
			temp = mac_to_string(tx_frame->get_source_mac()) + " " + mac_to_string(tx_frame->get_destination_mac());			
			temp += " ARP " + to_string(tx_frame->get_size()) + " ";
			// check to see if this is a request or a reply
			switch(get_ARP_opcode(&payload)) {
				case ARP_REQUEST:
					// generate the "who has" statement
					temp += "\"Who has " + ip_to_string(&payload, 18) + "? Tell " + ip_to_string(&payload, 8) + "\"";
					break;
				case ARP_REPLY:
					// genetare the "is at" statement
					temp += "\"" + ip_to_string(&payload, 8) + " is at " + mac_to_string(&payload,2) + "\"";
					break;
			}
			break;
		case MPDU_IP_TYPE:
			temp = ip_to_string((payload[4] << 24) + (payload[5] << 16) +
					(payload[6] << 8) + (payload[7])) + " ";
			temp += ip_to_string((payload[8] << 24) + (payload[9] << 16) +
					(payload[10] << 8) + (payload[11])) + " ";
			// this is an IP, so far we have three types, ping, TCP and UDP
			switch(get_IP_protocol(&payload)) {
				case IP_PROTOCOL_ICMP:
					temp += " ICMP " + to_string(tx_frame->get_size()) + " \"Echo (ping) ";
					// check to see if it's a request or reply
					switch(get_ping_type(&payload)) {
						case ICMP_ECHO_REQUEST:
							temp += "request\t";
							break;
						case ICMP_ECHO_REPLY:
							temp += "reply\t";
							break;
					}
					temp +="id=" + to_string((payload[IP_HEADER_SIZE + 4] < 8) + payload[IP_HEADER_SIZE + 5]) + ", seq=" +
							to_string((payload[IP_HEADER_SIZE + 6] < 8) + payload[IP_HEADER_SIZE + 7]) + "\""; 
					break;
				case IP_PROTOCOL_TCP:
					temp += " TCP " + to_string(tx_frame->get_size()) + "\t\"";
					// cout << to_string(payload[IP_HEADER_SIZE] << 8) << " " << to_string(payload[IP_HEADER_SIZE + 1]) << endl;
					temp += to_string((payload[IP_HEADER_SIZE] << 8) + payload[IP_HEADER_SIZE + 1]) + "->" + 
							to_string((payload[IP_HEADER_SIZE+2] << 8) + payload[IP_HEADER_SIZE + 3]) + " ";
					temp += "Seq=" + to_string(((uint32_t)(payload[IP_HEADER_SIZE +4]) << 24) + 
						((uint32_t)(payload[IP_HEADER_SIZE + 5]) << 16) +
						((uint32_t)(payload[IP_HEADER_SIZE + 6]) << 8) + 
						payload[IP_HEADER_SIZE + 7]) + "\t";
					temp += "Ack=" + to_string(((uint32_t)(payload[IP_HEADER_SIZE +8]) << 24) + 
						((uint32_t)(payload[IP_HEADER_SIZE + 9]) << 16) +
						((uint32_t)(payload[IP_HEADER_SIZE + 10]) << 8) + 
						payload[IP_HEADER_SIZE + 11]) + "\"";
					break;
				case IP_PROTOCOL_UDP:
					if (((payload[IP_HEADER_SIZE] << 8) + (payload[IP_HEADER_SIZE + 1]) == BOOTP_CLIENT_PORT) ||
						((payload[IP_HEADER_SIZE] << 8) + (payload[IP_HEADER_SIZE + 1]) == BOOTP_SERVER_PORT)) {
						temp += " DHCP " + to_string(tx_frame->get_size()) + "\t\"";
						switch(payload[IP_HEADER_SIZE + UDP_HEADER_SIZE]) {
							case DHCP_DISCOVER:
								temp += "DHCP Discover\"";
								break;
							case DHCP_OFFER:
								temp += "DHCP Offer\"";
								break;
							case DHCP_REQUEST:
								temp += "DHCP Request\"";
								break;
							case DHCP_ACK:
								temp += "DHCP ACK\"";
								break;
							case DHCP_NACK:
								temp += "DHCP NACK\"";
								break;
						}
					} else {
						temp += " UDP " + to_string(tx_frame->get_size()) + "\t\"";
						// cout << to_string(((uint32_t)(payload[IP_HEADER_SIZE])) << 8) << " " << to_string(payload[IP_HEADER_SIZE + 1]) << endl;
						temp += "Source port: " + to_string((payload[IP_HEADER_SIZE] << 8) + payload[IP_HEADER_SIZE + 1]) + " ";
						temp += "Destination port: " + to_string((payload[IP_HEADER_SIZE+ 2] << 8) + 
								payload[IP_HEADER_SIZE + 3]) + "\"";
					}
					break;
			}
			break;
	}

	string * result = new string(temp);
	return result;

}