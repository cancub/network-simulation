#include <stdlib.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include "host.h"
#include "pdu.h"
#include "addressing.h"
#include "wqueue.h"
#include "data_links.h"
#include "networking_devices.h"
#include <condition_variable> // std::condition_variable
#include <list>
#include <cstdint>

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

// void multiple_switch_test(int number_of_hosts, int number_of_switches) {

//     std::vector<std::vector<uint8_t>> mac_addresses;
//     std::vector<Switch*> switches;
//     std::vector<Host*> hosts;
//     std::vector<std::thread*> switch_threads;
//     std::vector<std::thread*> host_threads;
//     uint8_t mac_byte;

//     // start up the router which includes a DHCP server

//     // instantiate the switches and connect them
//     for (int i = 0; i < number_of_switches; i++) {
//         Switch* new_switch = new Switch("Switch" + std::to_string(i));
//         if (i == 1){
//             cout << "Plugging Switch 1 into Switch 0" << endl;
//             switches.front()->plug_in_device(new_switch);
//         } else if (i >= 2) {
//             int target_switch = rand() % switches.size();
//             switches.at(target_switch)->plug_in_device(new_switch);
//             cout << "Plugging Switch " << i << "into Switch " << target_switch << endl;
//         }
//         switches.push_back(new_switch);
//     }


//     // instantiate the hosts and connect them to the switches at random
//     for (int i = 0; i < number_of_hosts; i++){
//         mac_byte = (uint8_t)i;
//         mac_addresses.push_back(create_uniform_mac(mac_byte));
//         Host* new_host = new Host(create_random_ip(), mac_addresses.back(), name_list[i]);
//         int target_switch = rand() % switches.size();
//         switches.at(target_switch)->plug_in_device(new_host);
//         cout << "Plugging host " << name_list[i] << " into switch " << target_switch << endl;
//         hosts.push_back(new_host);
//     }

//     // wait until the user is ready to start the system
//     int ch = getchar();

//     // prepare the switch threads
//     for (int i = 0; i < number_of_switches; i++) {
//         std::thread* new_thread = new std::thread(&Switch::run, switches.at(i));
//         switch_threads.push_back(new_thread);
//     }

//     // prepare the host threads
//     for (int i = 0; i < number_of_hosts; i++) {
//         std::thread* new_thread = new std::thread(&Host::run, hosts.at(i), &mac_addresses);
//         host_threads.push_back(new_thread);
//     }

//     // start the switches
//     for (int i = 0; i < number_of_switches; i++) {
//         switch_threads.at(i)->join();
//     }

//     // start the hosts
//     for (int i = 0; i < number_of_hosts; i++) {
//         host_threads.at(i)->join();
//     }

//     // kill the hosts
//     while (hosts.size() > 0) {
//         host_threads.pop_back();
//         hosts.pop_back();
//     }

//     // kill the switches
//     while (switches.size() > 0) {
//         switch_threads.pop_back();
//         switches.pop_back();
//     }

//     // kill the router


// }

void arp_test() {
    // create a switch
    Switch* test_switch = new Switch("Switch1");

    cout << "creating MACs" << endl;
    std::vector<uint8_t> alice_mac = create_uniform_mac(0x11);
    std::vector<uint8_t> bob_mac = create_uniform_mac(0x22);
    // cout << "macs are " << mac_to_string(alice_mac) << " and " << mac_to_string(bob_mac) << endl;

    cout << "creating IPs" << endl;
    uint32_t alice_ip = create_ip(111,111,111,111);
    uint32_t bob_ip = create_ip(222,222,222,222);

    // create two hosts, giving each a distinct ip and mac
    Host* alice = new Host(alice_ip, alice_mac, "alice");
    Host* bob = new Host(bob_ip, bob_mac, "bob");

    // plug in the hosts to the switch
    cout << "plugging in hosts" << endl;
    test_switch->plug_in_device(alice);
    test_switch->plug_in_device(bob);


    // start the switch thread
    cout << "starting switch" << endl;
    std::thread* switch_thread = new std::thread(&Switch::run, test_switch);

    // test the arp capabilities of each host
    cout << "starting hosts" << endl;
    std::thread* alice_thread = new std::thread(&Host::run, alice, bob_ip);
    std::thread* bob_thread = new std::thread(&Host::run, bob, create_broadcast_ip());


    switch_thread->join();
    alice_thread->join();
    bob_thread->join();

}

int main(void) {
    srand (time(NULL));

    arp_test();

    // multiple_switch_test(2,1);
}