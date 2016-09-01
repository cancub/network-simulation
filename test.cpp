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

    std::string alice_mac = random_mac();
    std::string bob_mac = random_mac();
    std::cout << "Alice: \t" << alice_mac << std::endl;
    std::cout << "Bob: \t" << bob_mac << std::endl;
    Host alice(random_ip(), alice_mac, "alice");
    Host bob(random_ip(), bob_mac, "bob");
    Switch my_switch("star_switch");

    my_switch.plug_in_device(&alice);
    my_switch.plug_in_device(&bob);

    // just try and connect one host to the other and see if that works

    // for the purposes of naming conventions, we'll assume that alice is uplink from bob
    // so that the 'downlink' interface is the one that bob is receiving on

    // test 1
    // Ethernet* downlink = new Ethernet;
    // Ethernet* uplink = new Ethernet;

    // alice.set_tx_interface(downlink);
    // bob.set_rx_interface(downlink);

    // alice.set_rx_interface(uplink);
    // bob.set_tx_interface(uplink);

    std::thread test_switch(&Switch::run, &my_switch);
    std::thread test_alice(&Host::run,&alice, bob_mac);
    std::thread test_bob(&Host::run, &bob, alice_mac);

    test_switch.join();
    test_alice.join();
    test_bob.join();
}