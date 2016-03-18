#include <string>
#include <mutex>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <unistd.h>
#include "networking_devices.h"
#include "host.h"
#include "frames.h"

using namespace std;

/*
switch creates mutexes and interfaces
switch receives a vector of hosts or has hosts given to it one by one
switch gives a mutex and an interface to each connected host
switch runs
switch checks all the interfaces in a round robin fashion
if a mutex is locked
*/

Switch::Switch() {
	connected_hosts.reserve(6);
	interfaces.reserve(6);
	interface_mutexes.reserve(6);
	switch_table.reserve(6);
}

Switch::Switch(std::string switch_name) {
	connected_hosts.reserve(6);
	interfaces.reserve(6);
	interface_mutexes.reserve(6);
	switch_table.reserve(6);
	name = switch_name;
}

Switch::Switch(std::vector<Host*> hosts_to_connect, std::string switch_name) {
	connected_hosts.reserve(6);
	interfaces.reserve(6);
	interface_mutexes.reserve(6);
	switch_table.reserve(6);
	name = switch_name;

	connected_hosts = hosts_to_connect;
}

Switch::~Switch() {
	for (int i = 0; i < switch_table.size(); i++) {
		delete switch_table.at(i);
	}
}

void Switch::plug_in_device(Host* host_to_connect) {
	Host* temp_host = host_to_connect;
	connected_hosts.push_back(temp_host);
}

void Switch::run() {

	for (int i = 0; i < connected_hosts.size(); i++) {
		interface_mutexes.push_back(new std::mutex);
		interfaces.push_back(new Frame);
		(connected_hosts.at(i))->set_interface(interfaces.at(i));
		(connected_hosts.at(i))->set_mutex(interface_mutexes.at(i));
	}


	while (1) {
		// usleep(1);
		for (int i = 0; i < interface_mutexes.size(); i++) {
			// cout << name <<": here" << i << endl;
			// if ((interfaces.at(i))->get_src_mac() != "") {
			if ((interface_mutexes.at(i))->try_lock()) {
					// release the mutex if the other host has not locked it
					// since it isn't time to send the frame yet and there's nothing to
					// read
					(interface_mutexes.at(i))->unlock();
					if ((interfaces.at(i))->get_frame_size() != 0) {
						process_frame(i);
					}
				}
			else {
				// process the frame
				// cout << name << ": processing frame at " << i << endl;
				process_frame(i);			
			}

				// diff = std::chrono::high_resolution_clock::now() - start_time;

			// now that the code is done waiting, it can attempt to send the frame
			// send_frame(rand() % 1500, dst_mac);
		}
	}

}

void Switch::print_routing_table() {
	if (switch_table.size() > 0) {
		for (int i = 0; i < switch_table.size(); i++) {
			cout << switch_table.at(i)->address << " : " << switch_table.at(i)->interface_number << endl;
		}

	}
}

void Switch::unicast(int out_if) {
	// cout << name << ": unicasting frame" << endl;	
	// recursively attempt to put the frame on the link
	// just in case there's a conflict
	(interface_mutexes.at(out_if))->lock();
	(interfaces.at(out_if))->set_src_mac(frame_copy->get_src_mac());
	(interfaces.at(out_if))->set_dst_mac(frame_copy->get_dst_mac());
	(interfaces.at(out_if))->set_frame_size(frame_copy->get_frame_size());
		// cout << frame_copy->get_frame_size() << endl;
		// std::cout << name << ": relayed frame from " << frame_copy->get_src_mac() << " to "
			// << frame_copy->get_dst_mac() << " of size " << frame_copy->get_frame_size() << endl;
			// << dst_mac << " at " << runtime << " s" << std::endl;
	(interface_mutexes.at(out_if))->unlock();		
	// }
	// else {
	// 	cout << name << ": dropping frame *******************************" << endl;
	// 	// usleep(10000000);		
	// }
	// else, drop the frame (this will be better when queues are introduced)
}

void Switch::broadcast() {
	// send frames on all interfaces except the in interface, in_if
	// 
	// cout << name << ": broadcasting frame" << endl;
	for (int i = 0; i < interface_mutexes.size(); i++) {
		// check if interface number, i, is in one of the TableEntry 
		// objects. if it is and the interface isn't the same as the in_interface
		// send on that interface. if it isn't in the table, send on that interface as well.
		// this follows from the fact that a table entry for this frame's sender interface
		// must have ben added and thus we send to all other interfaces that have been added.
		if (get_table_interface_address(i) != frame_copy->get_src_mac()) {
			// cout << name << ": sending on interface " << i << endl;
			unicast(i);
		}
	}
}

void Switch::mutex_sleep() {
	// usleep(rand() % 10);
	usleep(1);
}

void Switch::process_frame(int interface_number) {
	std::string source_mac, destination_mac;
	int out_if_number = 0,  in_if_number = 0, f_size;
	// std::chrono::duration<double> diff;
	// increase the frame count
	// hang here to lock the mutex as soon as the host is done with it
	(interface_mutexes.at(interface_number))->lock();
	if (interfaces.at(interface_number)->get_frame_size() != 0) {

		// cout << "Processing frame at interface " << interface_number << endl;
		
		frame_copy = interfaces.at(interface_number);
		f_size = frame_copy->get_frame_size();
		// cout << f_size << endl;
		source_mac = frame_copy->get_src_mac();
		destination_mac = frame_copy->get_dst_mac();
		out_if_number = get_table_interface_number(destination_mac);
		in_if_number = get_table_interface_number(source_mac);

		// cout << name << ": in_interface = " << in_if_number << ", out_interface = " << out_if_number << endl;
		// print_routing_table();

		if (in_if_number == -1) {
			add_table_entry(source_mac, interface_number);
		}

		if (out_if_number >= 0) {
			// cout << name << ": unicasting " << interfaces.at(interface_number)->get_frame_size() 
			// 	<< " bytes from " << source_mac << " to " << destination_mac << endl;
			unicast(out_if_number);
		}
		else {
			// cout << name << ": broadcasting" << endl;
			broadcast();
		}
		// cout << name << ": processing frame, source_mac = " << source_mac << ", destination_mac = "
		// 	<< destination_mac << endl;
		(interfaces.at(interface_number))->erase();

	}
	// give the mutex back
	(interface_mutexes.at(interface_number))->unlock();
	// check the frame to see who it is from
	// cout << "sourc mac: " << source_mac << endl;
	// add it 

	// in_if = get_interface_number(source_mac);

	// and add their mac and the interface number to the switch table



	// if ((interface.at(i))->get_dst_mac() == mac) {
	// 	// diff = std::chrono::high_resolution_clock::now() - host_start_time;
	// 	// print some details about the frame
	// 	// increment_frame_count();
	// 	// cout << get_frame_count() << " " << mac << ": " << diff.count() << " frame of "
	// 	// 	<< interface->get_frame_size() << " bytes received from " << interface->get_src_mac() << endl;
	// 	// get rid of the frame to signify that it was received

		
	// }		
}

int Switch::get_table_interface_number(string interface_name) {
	int result = -1;
	if (switch_table.size() != 0) {
		for (int i = 0; i < switch_table.size(); i++) {
			if ((switch_table.at(i))->address == interface_name)
				result = (switch_table.at(i))->interface_number;
		}
	}

	return result;
}

std::string Switch::get_table_interface_address(int if_number) {
	std::string result = "";
	// cout << "table size:" << switch_table.size() << endl;
	// cout << name << ": looking for address at if number " << if_number << endl;

	if (switch_table.size() != 0) {
		for (int i = 0; i < switch_table.size(); i++) {
			if ((switch_table.at(i))->interface_number == if_number)
				result = (switch_table.at(i))->address;
		}
	}

	// cout << name << ": result = " << result << endl;

	return result;
}

void Switch::add_table_entry(std::string source_mac, int if_number) {
	TableEntry* new_entry = new TableEntry;
	new_entry->interface_number = if_number;
	new_entry->address = source_mac;
	switch_table.push_back(new_entry);
	// std::cout << "table entry added for " << source_mac << std::endl;

	// print_routing_table();
}
