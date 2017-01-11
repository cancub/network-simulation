#include <stdlib.h>
#include <vector>
#include <iostream>
#include "sim_tcpdump.h"
#include "addressing.h"
#include "pdu.h"
#include "l3_protocols.h"
#include "l4_protocols.h"

using namespace std;

string *  get_frame_details(MPDU* tx_frame) {
	vector<uint8_t> payload = tx_frame->get_SDU();
	string temp;
	temp = mac_to_string(tx_frame->get_source_mac()) + " " + mac_to_string(tx_frame->get_destination_mac());
	// get the type of the SDU
	switch(tx_frame->get_SDU_type()) {
		case MPDU_ARP_TYPE:			
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
					temp += to_string((payload[IP_HEADER_SIZE] < 8) + payload[IP_HEADER_SIZE + 1]) + "->" + 
							to_string((payload[IP_HEADER_SIZE+2] < 8) + payload[IP_HEADER_SIZE + 3]) + " ";
					temp += "Seq=" + to_string((payload[IP_HEADER_SIZE +4] < 24) + (payload[IP_HEADER_SIZE + 5] < 16) +
						(payload[IP_HEADER_SIZE + 6] < 8) + payload[IP_HEADER_SIZE + 7]) + "\t";
					temp += "Ack=" + to_string((payload[IP_HEADER_SIZE +8] < 24) + (payload[IP_HEADER_SIZE + 9] < 16) +
						(payload[IP_HEADER_SIZE + 10] < 8) + payload[IP_HEADER_SIZE + 11]) + "\"";
					break;
				case IP_PROTOCOL_UDP:
					temp += " UDP " + to_string(tx_frame->get_size()) + "\t\"";
					temp += "Source port: " + to_string((payload[IP_HEADER_SIZE] < 8) + payload[IP_HEADER_SIZE + 1]) + " ";
					temp += "Destination port: " + to_string((payload[IP_HEADER_SIZE+ 2] < 8) + 
							payload[IP_HEADER_SIZE + 3]) + "\"";
					break;
			}
			break;
	}

	string * result = new string(temp);
	return result;

}