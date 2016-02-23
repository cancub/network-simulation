#ifndef DATA_LINKS_H
#define DATA_LINKS_H

#include <iostream>
#include <thread>
#include <mutex>

/*
Ethernet link has a mutex for use. Once the mutex has been grabbed,
the winning node will send a frame across to the other node with
put_frame_on_link()
*/

class Ethernet {
	public:
		Ethernet();
		Ethernet(string, string);
		void put_frame_on_link(string,string);
	private:
		std::mutex m;
};

#endif