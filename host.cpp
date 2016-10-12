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
#include <cstdint>
#include "data_links.h"
#include "addressing.h"

using namespace std;

#define POISSON 10 // the default lambda that we'll work with for now
#define MTU     1500    // the maximum transmission unit describes how long a frame can be
#define FRAME_SIZE_MIN 60  
#define BROADCAST_MAC "FF:FF:FF:FF:FF:FF"

// #define DEBUG
// #define TEST
#define NUMBER_OF_FRAMES 100 // the number of frames to be sent by the host in a test


Host::Host() {
    rx_frame_count = 0;
    frame_generator = new Poisson(POISSON);
    host_start_time = std::chrono::high_resolution_clock::now();
    name = "unnamed";
#ifdef DEBUG
    host_print("Online");
#endif
}

Host::Host(std::vector<uint8_t> ip_addr, std::vector<uint8_t> mac_addr, std::string hostname) {
    ip = ip_addr;
    mac = mac_addr;
    rx_frame_count = 0;
    name = hostname;
    frame_generator = new Poisson(POISSON);
    host_start_time = std::chrono::high_resolution_clock::now();
#ifdef DEBUG
    // host_print("Online with MAC " + mac);
#endif
}


Host::~Host() {}

void Host::run(std::vector<std::vector<uint8_t>>* mac_list) {
    // make the threads for the sending and receiving interfaces
    std::thread receive_thread(&Host::receiver,this);
    std::thread send_thread(&Host::sender, this, mac_list);

    // let them run
    receive_thread.join();
    send_thread.join();
}

std::vector<uint8_t> Host::get_ip() {return ip;}

std::vector<uint8_t> Host::get_mac() {return mac;}

int Host::get_frame_count() {return rx_frame_count;}

void Host::set_ip(std::vector<uint8_t> ip_addr) {
    ip = ip_addr;
}

void Host::set_mac(std::vector<uint8_t> mac_addr) {
    mac = mac_addr;
}

void Host::set_port(Ethernet* tx_if, Ethernet* rx_if) {
    // a port can be defined by its outgoing and incoming wires for the ethernet link
    tx_interface = tx_if;
    rx_interface = rx_if;
}

void Host::sender(std::vector<std::vector<uint8_t>>* mac_list) {
    // this is to be run in a thread that will monitor the frame queue and
    // then interact with a specific interface once a frame becomes available
    // for sending

    int delay_us;                   // the amount of time the host will wait in microseconds
    int self_mac_element_id;        // the element of the mac list that contains this hosts's MAC
    int receiver_mac_element_id;    // the element of the mac list that contains the receiving host's mac
    int frame_size;                 // size of the frame being transmitted

    std::vector<uint8_t> dst_mac;

    // find the self id in the mac list and remember the location
    for(int i = 0; i < mac_list->size(); i++) {
        if (compare_macs(mac, mac_list->at(i)) == 0) {
            self_mac_element_id = i;
            break;
        }
    }

#ifdef TEST
    for(int i = 0; i < NUMBER_OF_FRAMES; i++) {
#else
    while(1) {
#endif
        // randomly select an element from the mac list
        while (1) {
            receiver_mac_element_id = rand() % mac_list->size();
            if (receiver_mac_element_id == self_mac_element_id) {
#ifdef TEST
                // do not send to this same host
                continue;
#else
                // send a broadcast frame if this host has selected its own MAC
                dst_mac = broadcast_mac();
                frame_size = 1;
                break;
#endif
            } else {
                // get the mac address associated with this element
                dst_mac = mac_list->at(receiver_mac_element_id);
                // choose random frame size that exists in range of 
                frame_size = (rand() % (MTU - FRAME_SIZE_MIN + 1)) + FRAME_SIZE_MIN;
                break;
            }
        }

        delay_us = frame_generator->interarrival_time_us();

#ifdef DEBUG 
        host_print("waiting " + std::to_string(delay_us/1000) + " ms");
#endif
        usleep(delay_us);

        // now that the sender is done waiting, it can attempt to send the frame.
        /*
        EDITOR'S NOTE: the frame size should be generated later on using probabilities
        related to REAL internet traffic. Maybe we could run a wireshark capture on a lab
        computer over several days to get a good idea. Ideally, this would even be time-of-
        day-dependent
        */
        send_frame(frame_size, dst_mac);
    }

 #ifdef TEST
    host_print("done sending");
 #endif   

}

void Host::receiver() {
    // this is to be run in a thread that will wait on a condition variable to check the rx
    // interface. 
    while (1) {
        process_frame(rx_interface->receive());
    }

}

void Host::send_frame(int frame_size, std::vector<uint8_t> dst_mac) {
    // a new frame is generated and is sent out on the link
    Frame* tx_frame = new Frame(mac, dst_mac,frame_size);

#ifdef DEBUG
    host_print("sending " +std::to_string(frame_size) + " bytes to " + dst_mac);
#endif
    // and that frame is sent out on the link
    tx_interface->transmit(tx_frame);
}

void Host::process_frame(Frame* rx_frame) {
    // the contents of received frames will be processed in this function

    std::chrono::duration<double> diff;

    std::vector<uint8_t> dst_mac = rx_frame->get_dst_mac();

    if (compare_macs(dst_mac,mac) == 0) {
        // this frame was destined for this host, so print some details about the frame

        diff = std::chrono::high_resolution_clock::now() - host_start_time;
        increment_frame_count();
        std::string statement = std::to_string(get_frame_count()) + " " + std::to_string(diff.count()) 
                                + " " + mac_to_string(rx_frame->get_src_mac())
                                 + " " + std::to_string(rx_frame->get_frame_size());
        host_print(statement);
       
    } else if (is_broadcast(dst_mac) == 0) {
        host_print("Received broadcast frame from " + mac_to_string(rx_frame->get_src_mac()));
    } else {
        host_print("received frame for different desination mac: " + mac_to_string(rx_frame->get_dst_mac()));
    }

    delete rx_frame;
}

void Host::increment_frame_count() {
    rx_frame_count += 1;
}

void Host::host_print(std::string statement) {
    std::cout << setw(15) << name << ": " << statement << endl;
}