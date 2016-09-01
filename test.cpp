#include <stdlib.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include "host.h"
#include "frames.h"
#include "addressing.h"
#include "wqueue.h"
#include "data_links.h"
#include "networking_devices.h"
#include <condition_variable> // std::condition_variable
#include <list>

using namespace std;

std::string name_list[26] = {
    "Alice",
    "Bob",
    "Carol",
    "Daniel",
    "Erica",
    "Frank",
    "Gwen", 
    "Henry",
    "Ida",
    "Joseph",
    "Karla",
    "Liam",
    "Madison",
    "Nate",
    "Ophelia",
    "Phil",
    "Quin",
    "Randy",
    "Sasha",
    "Tim",
    "Ursula",
    "Victor",
    "Wilma",
    "Xavier",
    "Yvette",
    "Zach"
};

void multiple_switch_test(int number_of_hosts, int number_of_switches) {

    std::vector<std::string> mac_addresses;
    std::vector<Switch*> switches;
    std::vector<Host*> hosts;
    std::vector<std::thread*> switch_threads;
    std::vector<std::thread*> host_threads;

    for (int i = 0; i < number_of_switches; i++) {
        Switch* new_switch = new Switch("Switch" + std::to_string(i));
        if (i == 1){
            cout << "Plugging Switch 1 into Switch 0" << endl;
            switches.front()->plug_in_device(new_switch);
        } else if (i >= 2) {
            int target_switch = rand() % switches.size();
            switches.at(target_switch)->plug_in_device(new_switch);
            cout << "Plugging Switch " << i << "into Switch " << target_switch << endl;
        }
        switches.push_back(new_switch);
    }

    for (int i = 0; i < number_of_hosts; i++){
        mac_addresses.push_back(random_mac());
        Host* new_host = new Host(random_ip(), mac_addresses.back(), name_list[i]);
        int target_switch = rand() % switches.size();
        switches.at(target_switch)->plug_in_device(new_host);
        cout << "Plugging host " << name_list[i] << " into switch " << target_switch << endl;
        hosts.push_back(new_host);
    }

    // Host bob(random_ip(), bob_mac, "bob");
    // Host carol(random_ip(), carol_mac, "carol");
    // Switch switch1("switch1");
    // Switch switch2("switch2");

    // switch1.plug_in_device(&alice);
    // switch1.plug_in_device(&switch2);
    // switch2.plug_in_device(&bob);
    // switch2.plug_in_device(&carol);

    // try to connect two switches now
    // plug in device should exchange interfaces
    // rx_interface = other_switch.set_tx_interface();

    for (int i = 0; i < number_of_switches; i++) {
        std::thread* new_thread = new std::thread(&Switch::run, switches.at(i));
        switch_threads.push_back(new_thread);
    }

    for (int i = 0; i < number_of_hosts; i++) {
        std::thread* new_thread = new std::thread(&Host::run, hosts.at(i), &mac_addresses);
        host_threads.push_back(new_thread);
    }

    // std::thread test_switch2(&Switch::run, &switch2);
    // std::thread test_alice(&Host::run,&alice, &mac_addresses);
    // std::thread test_bob(&Host::run, &bob, &mac_addresses);
    // std::thread test_carol(&Host::run, &carol, &mac_addresses);

    // test_switch.join();
    // test_switch1.join();
    // test_switch2.join();
    // test_alice.join();
    // test_bob.join();
    // test_carol.join();

    for (int i = 0; i < number_of_switches; i++) {
        switch_threads.at(i)->join();
    }

    for (int i = 0; i < number_of_hosts; i++) {
        host_threads.at(i)->join();
    }

    while (switches.size() > 0) {
        // std::thread* switch_thread_to_delete = switch_threads.back();
        switch_threads.pop_back();
        // delete switch_thread_to_delete;

        // Switch* switch_to_delete = switches.back();
        switches.pop_back();
        // delete switch_to_delete;
    }

    while (hosts.size() > 0) {
        // std::thread* host_thread_to_delete = host_threads.back();
        host_threads.pop_back();
        // delete host_thread_to_delete;

        // Host* host_to_delete = hosts.back();
        hosts.pop_back();
        // delete host_to_delete;
    }
}

int main(void) {
    srand (time(NULL));

    multiple_switch_test(10,3);
}