#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <string>
#include "frame_generators.h"
#include "host.h"

using namespace std;

Host::Host() {
	ip.reserve(4);
	mac_addr.reserve(6);
}

Host::Host(string ip_addr, string mac_addr) {
	ip = ip_addr;
	mac = mac_addr;
}

void Host::initialize() {}

void Host::start() {}

string Host::ip() {return ip;}

string Host::mac() {return mac;}

void send_frames(int seconds) {

	float total_time = 0.0;
	float arrival_time = 0.0;

	do {
		arrival_time = frame_generator.poisson_arrival();
		total_time += arrival_time;
		cout << "Frame sent at " << total_time << " s" << endl;
	} while (total_time < float(seconds));
}
