#ifndef DATA_LINKS_H
#define DATA_LINKS_H

#include <iostream>
#include <thread>
#include <mutex>
#include <string>

/*
Ethernet link has a mutex for use. Once the mutex has been grabbed,
the winning node will send a frame across to the other node with
put_frame_on_link()
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
		Ethernet(std::string, std::string);
		void run();
	private:
		std::mutex m;
		std::vector<int*> interfaces(2,0);	// these are Alice and Bob's interfaces
		std::vector<std::string> nodes(2,""); // the MACS of Alice and Bob
		int move_frame();
};

class Air {};

#endif