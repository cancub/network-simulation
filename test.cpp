#include <stdlib.h>
#include <thread>
#include <mutex>
#include <iostream>
#include "host.h"
#include "frames.h"
#include "addressing.h"
#include "networking_devices.h"

using namespace std;

int main(void) {
	srand (time(NULL));

	int i = 0;

	std::string alice_mac = random_mac();
	std::string bob_mac = random_mac();
	Host alice(random_ip(), alice_mac, "alice");
	Host bob(random_ip(), bob_mac, "bob");
	Switch my_switch("star_switch");

	// std::cout << i++ << std::endl;

	my_switch.plug_in_device(&alice);
	my_switch.plug_in_device(&bob);

	// std::cout << i++ << std::endl;
	thread test_switch(&Switch::run, &my_switch);
	// std::cout << i++ << std::endl;
	thread test_alice(&Host::run,&alice, bob_mac);
	// std::cout << i++ << std::endl;
	thread test_bob(&Host::run, &bob, alice_mac);

	// std::cout << i++ << std::endl;
	test_switch.join();
	test_alice.join();
	test_bob.join();
}