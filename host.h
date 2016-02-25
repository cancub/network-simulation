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
		string ip();
		string mac();
		void send_frames();
	private:
		Poisson frame_generator;
		string ip;
		string mac;
};

#endif