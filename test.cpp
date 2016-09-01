#include <stdlib.h>
#include <thread>
#include <mutex>
#include <iostream>
#include "host.h"
#include "frames.h"
#include "addressing.h"
#include "wqueue.h"
#include "data_links.h"
#include "networking_devices.h"
#include <condition_variable> // std::condition_variable

// using namespace std;

int main(void) {
    srand (time(NULL));

    int i = 0;

    std::vector<std::string> mac_list;

    std::string alice_mac = random_mac();
    mac_list.push_back(alice_mac);
    std::string bob_mac = random_mac();
    mac_list.push_back(bob_mac);
    std::string carol_mac = random_mac();
    mac_list.push_back(carol_mac);
    std::cout << "Alice: \t" << alice_mac << std::endl;
    std::cout << "Bob: \t" << bob_mac << std::endl;
    std::cout << "Carol: \t" << carol_mac << std::endl;
    Host alice(random_ip(), alice_mac, "alice");
    Host bob(random_ip(), bob_mac, "bob");
    Host carol(random_ip(), carol_mac, "carol");
    Switch my_switch("switch1");
    // Switch switch2("switch2");

    my_switch.plug_in_device(&alice);
    my_switch.plug_in_device(&bob);
    my_switch.plug_in_device(&carol);

    // try to connect two switches now
    // plug in device should exchange interfaces
    // rx_interface = other_switch.set_tx_interface();

    std::thread test_switch(&Switch::run, &my_switch);
    std::thread test_alice(&Host::run,&alice, &mac_list);
    std::thread test_bob(&Host::run, &bob, &mac_list);
    std::thread test_carol(&Host::run, &carol, &mac_list);

    test_switch.join();
    test_alice.join();
    test_bob.join();
    test_carol.join();
}