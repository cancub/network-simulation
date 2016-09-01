#ifndef NETWORKING_DEVICES_H
#define NETWORKING_DEVICES_H

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include "frames.h"
#include "host.h"
#include <condition_variable> // std::condition_variable
#include "wqueue.h"

class TableEntry {
    public:
        std::string address;
        int interface_number;
};

class Switch {
    public:
        Switch();
        Switch(std::string);
        Switch(std::vector<Host*>, std::string);
        ~Switch();
        void plug_in_device(Host*);
        int test_connection(int id);
        void run();
        void print_routing_table();
    private:
        void sender();
        void unicast(Frame*, int);
        void broadcast(Frame*);
        void receiver(int);
        void process_frame(Frame*, int);
        int get_table_interface_number(std::string);
        std::string get_table_interface_address(int);
        void add_table_entry(std::string, int);
        int total_ifs;
        std::vector<Host*> connected_hosts;
        std::vector<Ethernet*> rx_interfaces;
        std::vector<Ethernet*> tx_interfaces;
        std::vector<TableEntry*> switch_table;
        std::vector<std::thread*> rx_thread_list;
        wqueue<Frame*>* frame_queue;
        std::string name;
};

// class Router {

// };

// class Hub {

// };

// class Gateway {

// };

// class AccessPoint {

// };

#endif