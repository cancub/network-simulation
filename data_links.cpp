#include <mutex>
#include <iostream>
#include "data_links.h"
#include "frames.h"
#include <condition_variable> // std::condition_variable
#include "wqueue.h"

Ethernet::Ethernet() {
    //initialize a queue as the interface and set the maximum size of the queue to however
    // many frames this interface can hold (in this case 1)
    interface = new wqueue<Frame*>;
    interface->set_max_size(1);
}

Ethernet::~Ethernet() {
    delete interface;
}


void Ethernet::transmit(Frame* tx_frame) {
	// wait on the interface to be cleared before sending a frame
	interface->add(tx_frame);
}

Frame* Ethernet::receive() {
	// wait for a signal to be received and then process the frames
	return interface->remove();
}

// create_star(Switch* star_switch, std::vector<Host*> hosts_to_connect) {
//  for (std::vector<Host*>::iterator it=hosts_to_connect.begin(); it != hosts_to_connect.end(); ++it) {
//      star_switch->plug_in_device()
//  }
// }

