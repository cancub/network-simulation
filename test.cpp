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

void multiple_switch_test(int number_of_hosts, int number_of_switches) {

    std::vector<std::vector<uint8_t>> mac_addresses;
    std::vector<Switch*> switches;
    std::vector<Host*> hosts;
    std::vector<std::thread*> switch_threads;
    std::vector<std::thread*> host_threads;
    uint8_t mac_byte;

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
        mac_byte = (uint8_t)i;
        mac_addresses.push_back(uniform_mac(mac_byte));
        Host* new_host = new Host(random_ip(), mac_addresses.back(), name_list[i]);
        int target_switch = rand() % switches.size();
        switches.at(target_switch)->plug_in_device(new_host);
        cout << "Plugging host " << name_list[i] << " into switch " << target_switch << endl;
        hosts.push_back(new_host);
    }

    int ch = getchar();

    for (int i = 0; i < number_of_switches; i++) {
        std::thread* new_thread = new std::thread(&Switch::run, switches.at(i));
        switch_threads.push_back(new_thread);
    }

    for (int i = 0; i < number_of_hosts; i++) {
        std::thread* new_thread = new std::thread(&Host::run, hosts.at(i), &mac_addresses);
        host_threads.push_back(new_thread);
    }

    for (int i = 0; i < number_of_switches; i++) {
        switch_threads.at(i)->join();
    }

    for (int i = 0; i < number_of_hosts; i++) {
        host_threads.at(i)->join();
    }

    while (switches.size() > 0) {
        switch_threads.pop_back();
        switches.pop_back();
    }

    while (hosts.size() > 0) {
        host_threads.pop_back();
        hosts.pop_back();
    }
}

int main(void) {
    srand (time(NULL));

    multiple_switch_test(2,1);
}