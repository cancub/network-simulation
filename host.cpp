#include <stdlib.h>
#include <iostream>
#include <vector>
#include <thread>
#include <cmath>
#include <algorithm>
#include <unistd.h>
#include <string>
#include <chrono>
#include "host.h"
#include "frame_generators.h"
#include "frames.h"
#include <iomanip>
#include "data_links.h"

using namespace std;

#define POISSON 10 // the default lambda that we'll work with for now

// #define DEBUG
#define TEST

Host::Host() {
    rx_frame_count = 0;
    frame_generator = new Poisson(POISSON);

    host_start_time = std::chrono::high_resolution_clock::now();
}

Host::Host(std::string ip_addr, std::string mac_addr, std::string hostname) {
    ip = ip_addr;
    mac = mac_addr;
    rx_frame_count = 0;
    name = hostname;
    frame_generator = new Poisson(POISSON);
    host_start_time = std::chrono::high_resolution_clock::now();
}

// Host::Host(std::string ip_addr, std::string mac_addr, std::string hostname, 
//     std::mutex* if_mutex, Frame* iface) {

//     ip = ip_addr;
//     mac = mac_addr;
//     interface_mutex = if_mutex;
//     interface = iface;
//     rx_frame_count = 0;
//     name = hostname;
//     frame_generator = new Poisson(POISSON);
//     host_start_time = std::chrono::high_resolution_clock::now();
// }

Host::~Host() {}

void Host::run(std::vector<std::string>* mac_list) {
    // make the threads for the sending and receiving interfaces
    // let them run
    std::thread receive_thread(&Host::receiver,this);
    std::thread send_thread(&Host::sender, this, mac_list);

    receive_thread.join();
    send_thread.join();
}

std::string Host::get_ip() {return ip;}

std::string Host::get_mac() {return mac;}

int Host::get_frame_count() {return rx_frame_count;}

void Host::set_ip(std::string ip_addr) {
    ip = ip_addr;
}

void Host::set_mac(std::string mac_addr) {
    mac = mac_addr;
}

void Host::set_rx_interface(Ethernet* rx_if) {
    rx_interface = rx_if;
}

void Host::set_tx_interface(Ethernet* tx_if) {
    tx_interface = tx_if;
}

void Host::sender(std::vector<std::string>* mac_list) {
    // this is to be run in a thread that will wait for a specific interarrival time
    // before sending a frame. The sender then sets a condition variable, representing 
    // the high voltage at the receiver end.

    int delay_us;

    int self_mac_element_id;

    int receiver_mac_element_id;

    for(int i = 0; i < mac_list->size(); i++) {
        if (mac.compare(mac_list->at(i)) == 0) {
            self_mac_element_id = i;
            break;
        }
    }

#ifdef TEST
    for(int i = 0; i < 10; i++) {
#else
    while(1) {
#endif
        while(1) {
            receiver_mac_element_id = rand() % mac_list->size();
            if (receiver_mac_element_id != self_mac_element_id) {
                break;
            }
        }

        delay_us = frame_generator->interarrival_time()*1000000;

#ifdef DEBUG 
        host_print("waiting " + std::to_string(delay_us/1000) + " ms");
#endif
        usleep(delay_us);

        // now that the code is done waiting, it can attempt to send the frame.
        // the frame will be between 60 and 1500 bytes.
        /*
        EDITOR'S NOTE: the frame size should be generated later on using probabilities
        related to REAL internet traffic. Maybe we could run a wireshark capture on a lab
        computer over several days to get a good idea. Ideally, this would even be time-of-
        day-dependent
        */
        send_frame((rand() % 1441) + 60, mac_list->at(receiver_mac_element_id));
    }

 #ifdef TEST
    host_print("done sending");
 #endif   

}

void Host::receiver() {
    // this is to be run in a thread that will wait on a condition variable to check the rx
    // interface. 
    while (1) {
        // consume:
        process_frame(rx_interface->receive());
    }

}

void Host::send_frame(int frame_size, std::string dst_mac) {
    Frame* tx_frame = new Frame(mac, dst_mac,frame_size);
    // tx_frame->set_src_mac(mac);
    // tx_frame->set_dst_mac(dst_mac);
    // tx_frame->set_frame_size(frame_size);
#ifdef DEBUG
    host_print("sending " +std::to_string(frame_size) + " bytes to " + dst_mac);
#endif
    tx_interface->transmit(tx_frame);
}

void Host::process_frame(Frame* rx_frame) {
    std::chrono::duration<double> diff;

    if (rx_frame->get_dst_mac() == mac) {
        diff = std::chrono::high_resolution_clock::now() - host_start_time;
        // print some details about the frame
        increment_frame_count();
        std::string statement = std::to_string(get_frame_count()) + " " + std::to_string(diff.count()) 
                                + " " + rx_frame->get_src_mac() + " " + std::to_string(rx_frame->get_frame_size());
        host_print(statement);
       
        // host_print(std::to_string(get_frame_count()) + " " << mac + ": " + std::to_string(diff.count()) + " frame of "
        //     + std::to_string(rx_interface->get_frame_size()) + " bytes received from " + rx_interface->get_src_mac());
        // // get rid of the frame to signify that it was received
    } else {
        host_print("received frame for different desination mac: " + rx_frame->get_dst_mac());
    }

#ifdef DEBUG 
    host_print("deleting frame");
#endif
    delete rx_frame;
#ifdef DEBUG 
    host_print("frame deleted");
#endif
}

void Host::increment_frame_count() {
    rx_frame_count += 1;
    // cout << rx_frame_count << endl;
}

void Host::host_print(std::string statement) {
    std::cout << setw(15) << name << ": " << statement << endl;
}
// int main(void) {

//  Host alice("192.168.0.188", "aa:bb:cc:dd:ee:ff");
//  std::mutex test_mutex;
//  Frame interface;
//  alice.set_mutex(&test_mutex);
//  alice.set_interface(&interface);
//  alice.run("00:11:22:33:44:55");
// }