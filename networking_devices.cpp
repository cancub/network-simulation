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

/*
This switch class is for conencting to other switches and hosts for relaying frames.
Devices are "plugged into" this switch (and this switch can be plugged into other switches). 
When a frame is sent by the device on the other end of an interface, one of many receiving threads
processes the frame and places it in a thread-safe queue. A single sending thread then
takes the frame and sends it to the interface associated with the destination mac address
according to a routing table (which is built up via inspecting source mac addresses)
*/

using namespace std;

#define NUMBER_OF_INTERFACES 6      // the assumed number of physical interfaces the switch has 


// #define DEBUG

Switch::Switch() {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<Frame*>;
    total_ifs = 0;
}

Switch::Switch(std::string switch_name) {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<Frame*>;
    name = switch_name;
    total_ifs = 0;
}

Switch::Switch(std::vector<Host*> hosts_to_connect, std::string switch_name) {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<Frame*>;
    name = switch_name;
    total_ifs = 0;
}

Switch::~Switch() {
    for (int i = 0; i < switch_table.size(); i++) {
        delete switch_table.at(i);
        delete rx_interfaces.at(i);
        delete tx_interfaces.at(i);
    }
}

void Switch::plug_in_device(Host* device_to_connect) {
    // when notified that a device has been plugged in,
    // the switch creates a new rx and tx interface that it associates with that
    // n'th port and provides those interfaces to the device that was
    // just connected

    tx_interfaces.push_back(new Ethernet);
    rx_interfaces.push_back(new Ethernet);

    // set the interfaces for the port on the other end (note that their tx and rx
    // are a mirror of this switch's tx and rx)
    device_to_connect->set_port(rx_interfaces.back(), tx_interfaces.back());
}

void Switch::set_port(Ethernet* tx_if, Ethernet* rx_if) {
    // a port can be defined by its outgoing and incoming wires for the ethernet link
    tx_interfaces.push_back(tx_if);
    rx_interfaces.push_back(rx_if);
}

void Switch::run() {
    // start up sending thread with this switch so that the thread can access the queue and
    // tx interfaces
    std::thread tx_thread(&Switch::sender, this);

    // start up receiving threads which can also view the queue and rx interfaces, specifying
    // which interface each thread should be concerned with
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
    // show the current routining tabel for the switch as it has been learned
    // by inspecting source mac addresses
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
    // send one frame on a specific interface

#ifdef DEBUG
    std::cout << setw(15) <<  "[Sender]" << ": sending frame out interface " << if_id << std::endl;
#endif

    // will block until that interface is free
    tx_interfaces.at(if_id)->transmit(tx_frame);
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
            unicast(tx_frame->copy(), i);
        }
    }
#ifdef DEBUG
    cout << "switch deleting tx frame" << endl;
#endif
    delete tx_frame;
#ifdef DEBUG
    cout << "tx frame deleted" << endl;
#endif
}

void Switch::receiver(int if_id) {
    Frame* rx_frame;
    string RxName = "[Receiver" + to_string(if_id) + "]";
    // continually loop in trying to obtain a new frame from the medium
    while (1) {
        rx_frame = rx_interfaces.at(if_id)->receive();
#ifdef DEBUG
        std::cout << setw(15) << RxName << ": received frame" << std::endl;
#endif
        process_frame(rx_frame, if_id);
    }
}

void Switch::process_frame(Frame* rx_frame, int in_if) {
    // add the frame to the queue of frames to be transmitted
    // check to see if the sending device is listed and add the device to the i/f table if its not there
    int if_id = get_table_interface_number(rx_frame->get_src_mac());

    if (if_id == -1) {
        // add interface to table if it does not yet exist there
        add_table_entry(rx_frame->get_src_mac(), in_if);
    }

    frame_queue->add(rx_frame);

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
    // returns the string address associated with this specific interface (this should be
    // a list of mac addresses in the future since switches can be attached to switches)
    std::string result = "";

    if (switch_table.size() != 0) {
        for (int i = 0; i < switch_table.size(); i++) {
            if ((switch_table.at(i))->interface_number == if_number)
                result = (switch_table.at(i))->address;
        }
    }
    return result;
}

void Switch::add_table_entry(std::string source_mac, int if_id) {
    // add a new entry to the routing table that associates an interface id
    // to the mac address

    TableEntry* new_entry = new TableEntry;
    new_entry->interface_number = if_id;
    new_entry->address = source_mac;
    switch_table.push_back(new_entry);
#ifdef DEBUG
    std::cout << "table entry added for " << source_mac << std::endl;
    print_routing_table();
#endif
}
