#include <mutex>
#include <iostream>
#include "data_links.h"
#include "frames.h"
#include <condition_variable> // std::condition_variable
#include "wqueue.h"

#define MAX_QUEUE_LENGTH 20

Ethernet::Ethernet() {
    //initialize both the interface and it's related mutex
    interface = new wqueue<Frame*>;
    interface->set_max_size(MAX_QUEUE_LENGTH);
}

Ethernet::~Ethernet() {
    //get rid of the mutex and interface upon deletion
    delete interface;
}


void Ethernet::transmit(Frame* tx_frame) {
	interface->add(tx_frame);
}

Frame* Ethernet::receive() {
	return interface->remove();
}

// create_star(Switch* star_switch, std::vector<Host*> hosts_to_connect) {
//  for (std::vector<Host*>::iterator it=hosts_to_connect.begin(); it != hosts_to_connect.end(); ++it) {
//      star_switch->plug_in_device()
//  }
// }

