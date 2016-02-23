#ifndef HOST_H
#define HOST_H

#include <stdlib.h>
#include <string>
#include <vector>
#include "frame_generators.h"

class Host{
	public:
		Host();
		Host(string, string);
		void initialize();
		void start();
		str::vector<int> ip();
		string ip_string();
		str::vector<int> mac_addr();
		string mac_addr_string();
	private:
		std::vector<int> ip;
		std::vector<int> mac_addr;
};

#endif