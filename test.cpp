#include "host.h"
#include <stdlib.h>
#include <thread>
#include <mutex>
#include "frames.h"

using namespace std;

int main(void) {
	std::mutex test_mutex;
	Frame interface;
	Host alice("192.168.0.188", "aa:bb:cc:dd:ee:ff", &test_mutex, &interface);
	Host bob("192.168.0.186", "00:11:22:33:44:55", &test_mutex, &interface);

	alice.set_mutex(&test_mutex);
	bob.set_mutex(&test_mutex);
	alice.set_interface(&interface);
	bob.set_interface(&interface);
	thread test_alice(&Host::run,&alice, "00:11:22:33:44:55");
	thread test_bob(&Host::run, &bob, "aa:bb:cc:dd:ee:ff");
	test_alice.join();
	test_bob.join();
}