#ifndef NETWORKING_DEVICES_H
#define NETWORKING_DEVICES_H

#include <vector>
#include <string>
#include <mutex>
#include "frames.h"
#include "host.h"

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
		void run();
		void print_routing_table();
	private:
		void process_frame(int);
		void send_frame(int);
		void unicast(int);
		void broadcast();
		void mutex_sleep();
		int get_table_interface_number(std::string);
		std::string get_table_interface_address(int);
		void add_table_entry(std::string, int);
		std::vector<Host*> connected_hosts;
		std::vector<Frame*> interfaces;
		std::vector<std::mutex*> interface_mutexes;
		std::vector<TableEntry*> switch_table;
		Frame* frame_copy; // will need to turn into a queue
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