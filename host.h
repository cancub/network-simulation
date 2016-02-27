#ifndef HOST_H
#define HOST_H

#include <string>
#include "frame_generators.h"
#include "frames.h"

class Host{
	public:
		Host();
		Host(std::string, std::string);
		~Host();
		// void initialize();
		void run();
		std::string get_ip();
		std::string get_mac();
		void set_ip(std::string);
		void set_mac(std::string);
	private:
		Poisson* frame_generator;
		Frame* frame;
		Frame* interface;
		std::string ip;
		std::string mac;
};

#endif