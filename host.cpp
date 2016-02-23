#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>
#include <string>
#include "frame_generators.h"
#include "host.h"

using namespace std;

Host::Host() {
	ip.reserve(4);
	mac_addr.reserve(6);
}

Host::Host(string, string) {

}

void Host::initialize() {}

void Host::start() {}

str::vector<int> Host::ip() {}

string Host::ip_string() {}

str::vector<int> Host::mac_addr() {}

string Host::mac_addr_string() {}
