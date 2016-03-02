#include <mutex>
#include "data_links.h"
#include "frames.h"

Ethernet::Ethernet() {
	//initialize both the interface and it's related mutex
	m = new std::mutex;
	interface = new Frame;
}

Ethernet::~Ethernet() {
	//get rid of the mutex and interface upon deletion
	delete m;
	delete interface;
}

// these are functions that the orchestration agent will use to 
// connect nodes to the link
std::mutex* get_mutex_pointer() {return m;}
Frame* get_interface_pointer() {return interface;}

// create_star(Switch* star_switch, std::vector<Host*> hosts_to_connect) {
// 	for (std::vector<Host*>::iterator it=hosts_to_connect.begin(); it != hosts_to_connect.end(); ++it) {
// 		star_switch->plug_in_device()
// 	}
// }

