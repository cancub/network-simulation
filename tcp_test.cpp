#include "l4_protocols.h"
#include "l3_protocols.h"
#include "pdu.h"
#include "addressing.h"
#include "wqueue.h"
#include <cstdint>
#include <thread>
#include <iostream>

using namespace std;

#define FIRST_IP 1
#define FIRST_PORT 20000
#define SECOND_IP 2
#define SECOND_PORT 30000
#define SYN 0x0002
#define ACK 0x0010

void TCP_tester(/*int my_IP; int dst_IP, */uint16_t my_port, uint16_t dst_port, int is_sender, wqueue<TCP*> * tx_interface,
wqueue<TCP*> * rx_interface) {
	// if is_sender is true, this is the tester that initiates the sync
	if (is_sender) {
		// generate a TCP frame with the SYN bit set to 1 and with a random value in the sequence number
		TCP * SYN_segment = new TCP;
		SYN_segment->source_port = my_port;
		SYN_segment->destination_port = dst_port;
		SYN_segment->sequence_number = my_port;
		SYN_segment->details = SYN;
		cout << my_port << " sending SYN" << endl;
		tx_interface->add(SYN_segment);
	}

	TCP * rcv_segment;
	TCP * send_segment;

	while (true) {
		rcv_segment = rx_interface->remove();
		if (rcv_segment->details == (SYN & ACK)) {
			delete rcv_segment;
			if (is_sender) {
				cout << my_port << " got SYNACK, ACKing" << endl;
				send_segment = new TCP;
				send_segment->destination_port = dst_port;
				send_segment->sequence_number = my_port;
				send_segment->details = SYN & ACK;
				tx_interface->add(send_segment);			
			}
			cout << my_port << " fully synced, exiting" << endl;
			break;
		} else if (rcv_segment->details == SYN) {
			cout << my_port << " got sync request, replying" << endl;
			send_segment = new TCP;
			send_segment->destination_port = dst_port;
			send_segment->sequence_number = my_port;
			send_segment->details = SYN & ACK;
			tx_interface->add(send_segment);
		}
	}



}

int main() {

	// create queue for each host to reference
	wqueue<TCP*> * interface_1 = new wqueue<TCP*>;
	wqueue<TCP*> * interface_2 = new wqueue<TCP*>;
	// have two threads which reference one queue
	std::thread server(TCP_tester, FIRST_PORT, SECOND_PORT, 0, interface_1, interface_2);
	std::thread client(TCP_tester, SECOND_PORT, FIRST_PORT, 1, interface_2, interface_1);

	server.join();
	client.join();


	// each

}