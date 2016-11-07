#ifndef HOST_H
#define HOST_H

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include "frame_generators.h"
#include "pdu.h"
#include <condition_variable> // std::condition_variable
#include "data_links.h"
#include "l3_protocols.h"


/*
This is one of the big players in the network simulation. Without hosts,
there's no reason for a network. Hosts need to generate frames, access
links and read frames. That's pretty much it at the beggining. Later on
we'll add something to react to specific situations or events, like a ping
*/

class Host{
    public:
        Host(); // default constructor
        Host(uint32_t, std::vector<uint8_t>, std::string); // IP, MAC, and name (like "alice")
        // Host(std::string, std::string, std::string, std::mutex*, MPDU*); // same as above but with a
                                                                            // mutex and interface
        ~Host();
        void run(uint32_t); // start sending frames to the specified MAC address
        uint32_t get_ip();
        std::vector<uint8_t> get_mac();
        int get_frame_count();
        void set_ip(uint32_t);
        void set_mac(std::vector<uint8_t>);
        void set_port(Ethernet*, Ethernet*);
        void muxer_demuxer();
        void receiver();
    private:
        void send_MPDU(MPDU*); // sending a frame to a specific MAC address
        void process_frame(MPDU*, std::vector<uint8_t>); // process an incoming frames
        void increment_frame_count();
        void fill_ping_payload();
        void run_ping_process(uint32_t, int);
        void receive_ping(std::vector<uint8_t>, uint32_t, std::vector<uint8_t>);
        void arping(uint32_t, int, int);
        void send_request_arp(uint32_t, int);
        void send_arp(ARP, std::vector<uint8_t>, int);
        void receive_arp(std::vector<uint8_t>);
        int run_DHCP_handshake();
        void host_print(std::string);
        ICMP ping_to_send;
        uint8_t ping_running;
        std::chrono::time_point<std::chrono::high_resolution_clock> ping_send_time;
        Poisson* frame_generator; // the object that will create all the interarrival times
        Ethernet* rx_interface; // the location where the frames will be found/put on
        Ethernet* tx_interface; // the location where the frames will be found/put on
        wqueue<MPDU*>* frame_queue;
        uint32_t ip;
        uint32_t netmask;
        std::vector<uint8_t> mac;
        std::vector<uint8_t> router_mac;
        std::chrono::time_point<std::chrono::high_resolution_clock> host_start_time; // keep track of time
        int rx_frame_count; // keep track of number of frames received
        std::string name; // the name of the host
        ARP_cache cache;
};

#endif