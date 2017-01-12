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
links and read frames. 
*/

class Host{
    public:
        Host(); // default constructor
        Host(uint32_t, std::vector<uint8_t>, std::string, mutex*); // IP, MAC, and name (like "alice")
        // Host(std::string, std::string, std::string, std::mutex*, MPDU*); // same as above but with a
                                                                            // mutex and interface
        ~Host();
        void arp_test(uint32_t, int);
        void ping_test(uint32_t, int, int delay = 0);
        void tcp_test(const char *, uint32_t, uint16_t, uint16_t, int); // start sending frames to the specified MAC address
        void udp_test(const char *, uint32_t, uint16_t, uint16_t, int); // start sending frames to the specified MAC address
        uint32_t get_ip();
        std::vector<uint8_t> get_mac();
        int get_frame_count();
        void set_ip(uint32_t);
        void set_mac(std::vector<uint8_t>);
        void connect_ethernet(EthernetLink* e_link, bool flip_wires = false);
        void demuxer();
        void receiver();
    private:
        void send_MPDU(MPDU*); // sending a frame to a specific MAC address
        void process_frame(MPDU*, std::vector<uint8_t>); // process an incoming frames
        void increment_frame_count();
        void fill_ping_payload(ICMP*);
        void ping(uint32_t, int);
        void print_ping_statistics(int, int,std::vector<float>);
        void process_ICMP_message(MPDU*);
        void arping(uint32_t, int, int);
        void send_request_arp(uint32_t, int);
        void send_arp(ARP, std::vector<uint8_t>);
        void receive_arp(std::vector<uint8_t>);
        int run_DHCP_handshake();
        void host_print(std::string);
        void TCP_client(const char *, uint16_t this_port, uint32_t dest_ip, uint16_t dest_port);
        void TCP_server(const char *filename, uint16_t this_port);
        void UDP_client(const char*, uint16_t this_port, uint32_t dest_ip, uint16_t dest_port);
        void UDP_server(const char *filename, uint16_t this_port);
        Socket* create_socket(uint16_t,uint8_t);         // create a socket that can be used for a specific process 
        void delete_socket(uint16_t);
        std::vector<Socket*>        open_ports; // move to smart pointers in the future
        mutex*                      m_mutex;
        Poisson*                    frame_generator; // the object that will create all the interarrival times
        EthernetWire*               rx_interface; // the location where the frames will be found/put on
        EthernetWire*               tx_interface; // the location where the frames will be found/put on
        wqueue<MPDU*>*              frame_queue;
        uint32_t                    ip;
        uint32_t                    netmask;
        std::vector<uint8_t>        mac;
        std::vector<uint8_t>        router_mac;
        std::chrono::time_point<std::chrono::high_resolution_clock> host_start_time; // keep track of time
        int                         rx_frame_count; // keep track of number of frames received
        std::string                 name; // the name of the host
        ARP_cache                   cache;
};

#endif