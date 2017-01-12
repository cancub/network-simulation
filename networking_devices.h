#ifndef NETWORKING_DEVICES_H
#define NETWORKING_DEVICES_H

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <ctime>
#include <cstdint>
#include <condition_variable> // condition_variable
#include "pdu.h"
#include "host.h"
#include "wqueue.h"

using namespace std;


#define DHCP_NO_ENTRY   0

#define DHCP_DISCOVER   1
#define DHCP_OFFER      2
#define DHCP_REQUEST    3
#define DHCP_ACK        5
#define DHCP_NACK       6

#define BOOTP_SERVER_PORT   67
#define BOOTP_CLIENT_PORT   68


class TableEntry {
    public:
        vector<vector<uint8_t>> address_list;
        int interface_number;
};

struct DHCP {
    uint8_t opcode; // request, offer, etc
    uint32_t client_ip; // the client's current ip
    uint32_t your_ip;   // the address that the server wants to assign to the client
};

struct DHCPEntry {
    vector<uint8_t> mac;
    uint32_t ip;
    uint8_t current_opcode;
    vector<uint8_t> first_hop_router;
    time_t creation_time;
};

DHCP generate_DHCP(vector<uint8_t> udp_u8);

class DHCPServer {
    public:
        DHCPServer(wqueue<MPDU*>*, EthernetWire*, vector<uint8_t>, uint32_t);
        void run();
        void set_subnet(uint32_t, uint32_t);
        void set_subnet_ip(uint32_t);
        void set_subnet_netmask(uint32_t);
    private:
        void flush_ips();
        DHCPEntry get_entry(vector<uint8_t>);
        void set_entry(vector<uint8_t> test_mac, uint32_t set_ip, uint8_t opcode);
        uint32_t subnet_ip;
        uint32_t gateway_ip;
        uint32_t netmask;
        uint32_t DNS_server;
        vector<DHCPEntry> table;
        time_t ip_lifetime;
        EthernetWire* tx_if;
        wqueue<MPDU*>* input_queue;
        vector<uint8_t> gw_mac;
        uint32_t gw_ip;
};

class Switch {
    public:
        Switch();
        Switch(string, mutex*);
        Switch(vector<Host*>, string);
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
        int get_table_interface_number(vector<uint8_t>);
        void add_table_entry(vector<uint8_t>, int);
        void switch_print(string);
        vector<Host*>              connected_hosts;
        vector<EthernetWire*>      rx_interfaces;
        vector<EthernetWire*>      tx_interfaces;
        vector<TableEntry*>        switch_table;
        vector<thread*>       rx_thread_list;
        wqueue<MPDU*>*                  frame_queue;
        mutex*                          mutex_m;
        string                     name;
};

class Gateway {
    public:
        Gateway(string, mutex*, vector<uint8_t>, uint32_t); 
        ~Gateway();
        void connect_ethernet(EthernetLink* e_link, bool flip_wires = false);
        void run();      
    private:
        void receiver();
        void gateway_print(string);
        DHCPServer* main_dhcp;
        // wqueue<MPDU*>* internet_rx_queue;
        // wqueue<MPDU*>* internet_tx_queue;
        EthernetWire* lan_rx_if;
        EthernetWire* lan_tx_if;    // this is where frames will be sent out, will be given to DHCP server
        wqueue<MPDU*>* process_queue; // received frames will be fed into here by the receiver
        wqueue<MPDU*>* DHCP_queue;    // used to send frames to the DHCP_server
        string name;
        mutex* mutex_m;
        vector<uint8_t> mac;
        uint32_t ip;
};



// class Hub {

// };

// class Router {};

// class AccessPoint {

// };

#endif