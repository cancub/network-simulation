#ifndef NETWORKING_DEVICES_H
#define NETWORKING_DEVICES_H

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include "frames.h"
#include "host.h"
#include <cstdint>
#include <condition_variable> // std::condition_variable
#include "wqueue.h"

class TableEntry {
    public:
        std::vector<std::vector<uint8_t>> address_list;
        int interface_number;
};

class Switch {
    public:
        Switch();
        Switch(std::string);
        Switch(std::vector<Host*>, std::string);
        ~Switch();
        void plug_in_device(Host*);
        void plug_in_device(Switch*);
        void set_port(Ethernet*, Ethernet*);
        void run();
        void print_routing_table();
    private:
        void sender();
        void unicast(Frame*, int);
        void broadcast(Frame*);
        void receiver(int);
        void process_frame(Frame*, int);
        int get_table_interface_number(std::vector<uint8_t>);
        void add_table_entry(std::vector<uint8_t>, int);
        void switch_print(std::string);
        std::vector<Host*> connected_hosts;
        std::vector<Ethernet*> rx_interfaces;
        std::vector<Ethernet*> tx_interfaces;
        std::vector<TableEntry*> switch_table;
        std::vector<std::thread*> rx_thread_list;
        wqueue<Frame*>* frame_queue;
        std::string name;
};

class Router {
    public:
        Router();        
    private:
        std::string formulate_ACK();
        void receiver();
        void sender();

};

class DHCPServer {

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