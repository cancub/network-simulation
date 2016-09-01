#include <string>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <unistd.h>
#include "networking_devices.h"
#include "host.h"
#include "frames.h"
#include <condition_variable> // std::condition_variable
#include "data_links.h"
#include <iomanip>
#include "wqueue.h"

using namespace std;

/*
switch creates oxes and interfaces
switch receives a vector of hosts or has hosts given to it one by one
switch gives a mutex and an interface to each connected host
switch runs
switch checks all the interfaces in a round robin fashion
if a mutex is locked
*/

Switch::Switch() {
    connected_hosts.reserve(6);
    rx_interfaces.reserve(6);
    tx_interfaces.reserve(6);
    switch_table.reserve(6);
    rx_thread_list.reserve(6);
    frame_queue = new wqueue<Frame*>;
    total_ifs = 0;
}

Switch::Switch(std::string switch_name) {
    connected_hosts.reserve(6);
    rx_interfaces.reserve(6);
    tx_interfaces.reserve(6);
    switch_table.reserve(6);
    rx_thread_list.reserve(6);
    frame_queue = new wqueue<Frame*>;
    name = switch_name;
    total_ifs = 0;
}

Switch::Switch(std::vector<Host*> hosts_to_connect, std::string switch_name) {
    connected_hosts.reserve(6);
    rx_interfaces.reserve(6);
    tx_interfaces.reserve(6);
    switch_table.reserve(6);
    rx_thread_list.reserve(6);
    frame_queue = new wqueue<Frame*>;
    name = switch_name;
    total_ifs = 0;
    connected_hosts = hosts_to_connect;
}

Switch::~Switch() {
    for (int i = 0; i < switch_table.size(); i++) {
        delete switch_table.at(i);
        delete rx_interfaces.at(i);
        delete tx_interfaces.at(i);
    }
}

void Switch::plug_in_device(Host* host_to_connect) {
    tx_interfaces.push_back(new Ethernet);
    rx_interfaces.push_back(new Ethernet);
    // set_rx_interface returns a tx_interface that is paired 
    host_to_connect->set_rx_interface(tx_interfaces.back());
    host_to_connect->set_tx_interface(rx_interfaces.back());
    connected_hosts.push_back(host_to_connect);
}

int Switch::test_connection(int id) {
    // check to see if the connection specified by the id number
    // is indeed functioning by sending a PING. Note that actual switches
    // would not verify that a connection is in place


}

void Switch::run() {


    // create main queue for communication of jobs between sending thread and receiving threads
    // wqueue<Frame*> main_queue;
    // start up sending thread with queue
    std::thread tx_thread(&Switch::sender, this);

    // start up receiving threads with queue and respective interface
    for (int i = 0; i < rx_interfaces.size(); i++) {
        std::thread* rx_thread = new std::thread(&Switch::receiver, this, i);
        rx_thread_list.push_back(rx_thread);
    }

    tx_thread.join();
    for (int i = 0; i < rx_interfaces.size(); i++) {
        rx_thread_list.at(i)->join();
    }
}

void Switch::print_routing_table() {
    if (switch_table.size() > 0) {
        for (int i = 0; i < switch_table.size(); i++) {
            cout << switch_table.at(i)->address << " : " << switch_table.at(i)->interface_number << endl;
        }

    }
}

void Switch::sender() {
    Frame* tx_frame;    // pointer to the frame to be sent out
    int if_id;          // the id of the interface that is sending the frame

    // continually check queue for frames
    while(1) {
        // get the next frame
        tx_frame = frame_queue->remove();
        // find if this frame is for a device that has been registered in the table
        if_id = get_table_interface_number(tx_frame->get_dst_mac());
        if (if_id != -1) {
            // send to the one interface associated with the device
            unicast(tx_frame, if_id);
        } else {
            // send to all interfaces
            broadcast(tx_frame);
        }  
    }
}

void Switch::unicast(Frame* tx_frame, int if_id) {
    // no need to do anything fancy, wqueue takes care of mutexes and condition variables
    std::cout << setw(15) <<  "[Sender]" << ": sending frame out interface " << if_id << std::endl;
    tx_interfaces.at(if_id)->transmit(tx_frame);
#ifdef DEBUG
    // host_print("sending " +std::to_string(tx_frame->get_frame_size()) + " bytes to " + tx_frame->get_dst_mac());
#endif
}

void Switch::broadcast(Frame* tx_frame) {
    // send frames on all interfaces except the in interface, in_if
    for (int i = 0; i < tx_interfaces.size(); i++) {
        // check if interface number, i, is in one of the TableEntry 
        // objects. if it is and the interface isn't the same as the in_interface
        // send on that interface. if it isn't in the table, send on that interface as well.
        // this follows from the fact that a table entry for this frame's sender interface
        // must have ben added and thus we send to all other interfaces that have been added.
        if (get_table_interface_address(i) != tx_frame->get_src_mac()) {
            // cout << name << ": sending on interface " << i << endl;
            unicast(tx_frame->copy(), i);
        }
    }

    delete tx_frame;
}

void Switch::receiver(int if_id) {
    string RxName = "[Receiver" + to_string(if_id) + "]";
    // continually loop in trying to obtain a new frame from the medium
    while (1) {
        process_frame(rx_interfaces.at(if_id)->receive());
        std::cout << setw(15) << RxName << ": received frame" << std::endl;
    }
}

void Switch::process_frame(Frame* rx_frame) {
    // add the frame to the queue of frames to be transmitted
    frame_queue->add(rx_frame);

    // check to see if the sending device is listed and add the device to the i/f table if its not there
    int in_if = get_table_interface_number(rx_frame->get_src_mac());

    if (in_if == -1) {
        // add interface to table if it does not yet exist there
        add_table_entry(rx_frame->get_src_mac());
    }
}

int Switch::get_table_interface_number(string mac_addr) {
    // obtain the interface number associatd with this device.
    // of note is that an interface can be associated with many devices, something that will
    // have to be updated in the code when we move to a true network format
    int result = -1;
    if (switch_table.size() != 0) {
        for (int i = 0; i < switch_table.size(); i++) {
            if ((switch_table.at(i))->address == mac_addr)
                result = (switch_table.at(i))->interface_number;
        }
    }

    return result;
}

std::string Switch::get_table_interface_address(int if_number) {
    std::string result = "";

    if (switch_table.size() != 0) {
        for (int i = 0; i < switch_table.size(); i++) {
            if ((switch_table.at(i))->interface_number == if_number)
                result = (switch_table.at(i))->address;
        }
    }
    return result;
}

void Switch::add_table_entry(std::string source_mac) {
    TableEntry* new_entry = new TableEntry;
    new_entry->interface_number = total_ifs++;
    new_entry->address = source_mac;
    switch_table.push_back(new_entry);
    std::cout << "table entry added for " << source_mac << std::endl;

    print_routing_table();
}
