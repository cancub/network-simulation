#ifndef NETWORKING_DEVICES_H
#define NETWORKING_DEVICES_H

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include "pdu.h"
#include "host.h"
#include <cstdint>
#include <condition_variable> // std::condition_variable
#include "wqueue.h"
#include <ctime>

class TableEntry {
    public:
        std::vector<std::vector<uint8_t>> address_list;
        int interface_number;
};

class DHCPEntry {
    std::vector<uint8_t> mac;
    uint32_t ip;
    std::vector<uint8_t> first_hop_router;
    std::time_t creation_time;
};

class DHCPServer {
    public:
        DHCPServer(wqueue<MPDU*>*);
        DHCPServer(wqueue<MPDU*>*, uint32_t, uint32_t);
        DHCPServer(wqueue<MPDU*>*, uint32_t, uint32_t, uint32_t);
        void set_subnet(uint32_t, uint32_t);
        void set_subnet_ip(uint32_t);
        void set_subnet_netmask(uint32_t);
        void set_DNS_server(uint32_t);
        void new_DHCP_discover(uint32_t, std::vector<uint8_t>);
    private:
        void send_DHCP_offer();
        std::vector<uint8_t> generate_ip();
        void flush_ips();
        uint32_t subnet_ip;
        uint32_t netmask;
        uint32_t DNS_server;
        std::vector<DHCPEntry> table;
        std::time_t ip_lifetime;
        wqueue<MPDU*>* tx_queue;
};

class Switch {
    public:
        Switch();
        Switch(std::string, mutex*);
        Switch(std::vector<Host*>, std::string);
        ~Switch();
        void connect_ethernet(EthernetLink* e_link, bool flip_wires);
        void run();
        void print_routing_table();
    private:
        void sender();
        void unicast(MPDU*, int);
        void broadcast(MPDU*);
        void receiver(int);
        void process_frame(MPDU*, int);
        int get_table_interface_number(std::vector<uint8_t>);
        void add_table_entry(std::vector<uint8_t>, int);
        void switch_print(std::string);
        std::vector<Host*>              connected_hosts;
        std::vector<EthernetWire*>      rx_interfaces;
        std::vector<EthernetWire*>      tx_interfaces;
        std::vector<TableEntry*>        switch_table;
        std::vector<std::thread*>       rx_thread_list;
        wqueue<MPDU*>*                  frame_queue;
        mutex*                          mutex_m;
        std::string                     name;
};

class Router {
    public:
        Router(); 
        Router(uint32_t, uint32_t, uint32_t);      
    private:
        std::string formulate_ACK();
        void receiver();
        void sender();
        DHCPServer* main_dhcp;
        wqueue<MPDU*>* frame_queue;
};



// class Hub {

// };

// class Gateway : public Switch {
//     public:
//         Gateway();
//         void set_
//     private:

// };

// class AccessPoint {

// };

#endif