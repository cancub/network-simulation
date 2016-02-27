#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>
#include <string>
#include "frame_generators.h"
#include "frames.h"
#include "host.h"
#include <chrono>

using namespace std;

Host::Host() {
	ip.reserve(4);
	mac.reserve(6);
}

Host::Host(std::string ip_addr, std::string mac_addr) {
	ip = ip_addr;
	mac = mac_addr;
	frame = new Frame(mac);
	frame_generator = new Poisson(10);
}

Host::~Host() {
	delete frame;
	delete frame_generator;
}

// void Host::initialize() {}

void Host::run() {
	// get frame delay
	// loop until then
	// in loop, check to see if mutex is grabbed
	// when grabbed, retrieve frame from interface and
	// process it
	// after loop finishes, grab mutex and put on interface
	// go back to beginning

	std::chrono::duration<double> delay_us;
	double total_t = 0.0;
	auto start_time = std::chrono::high_resolution_clock::now();

	while (1) {
		start_time = std::chrono::high_resolution_clock::now();
		delay_us = std::chrono::duration<double>(frame_generator->interarrival_time());

		while (std::chrono::high_resolution_clock::now() - start_time < delay_us) {
			// if(/*mutex has been grabbed*/) {
			// 	/*wait on mutex*/
			// 	/*grab mutex*/
			// 	copy frame
			// 	/*clear frame*/
			// 	/*release mutex*/
			// 	/*print copy*/
			// }
		}

		total_t += delay_us.count();

		std::cout << "Frame arrived at " << total_t << " s" << std::endl;
		/*grab mutex*/
		/*put put frame on link*/
		/*release mutex*/

	}
}

std::string Host::get_ip() {return ip;}

std::string Host::get_mac() {return mac;}

void Host::set_ip(std::string ip_addr) {
	ip = ip_addr;
}

void Host::set_mac(std::string mac_addr) {
	mac = mac_addr;
}

int main(void) {

	Host alice("192.168.0.188", "aa:bb:cc:dd:ee:ff");
	alice.run();
}