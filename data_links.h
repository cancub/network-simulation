#ifndef DATA_LINKS_H
#define DATA_LINKS_H

#include <mutex>
#include <vector>
#include "frames.h"

/*
These are the links that will be used to connect nodes in the network.
There will likely only be two links: Ethernet and air.
Ethernet should be straight forward in that it's a passive object
that just has an interface that nodes can add a frame to
or retrieve a frame from, and there is a mutex that must be locked
to interact with this interface. Air will be a bit different as there
should be delay depending on the location of the station
and there should also be a possibility of collision on the link.

Ethernet should be agnostic to who or what is trying to interact with it.
It should simply be a well-described object that both ends of the link
can actively engage while it just sits there.
*/



class Ethernet {
	/*
	process for frame sending is: 
	1) alice claims mutex
	2) alice places int value representing frame size at her interace
	3) link takes it and places it on bob's interface
	4) bob has noticed that the mutex was taken and so has been waiting to
		receive frame
	5) bob sees frame at interface, processes it and immediately attempts to place frame on link
	6) go to 1) 
	*/
	public:
		Ethernet();
		~Ethernet();
		std::mutex* get_mutex_pointer();
		Frame* get_interface_pointer();
	private:
		std::mutex* m;
		Frame* interface;
};

class Air {};

// create_star(Switch*, std::vector<Host*>);

#endif