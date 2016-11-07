#include <string>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <unistd.h>
#include "networking_devices.h"
#include "host.h"
#include "pdu.h"
#include <condition_variable> // std::condition_variable
#include "data_links.h"
#include <iomanip>
#include "wqueue.h"
#include "addressing.h"
#include <ctime>

/*
This switch class is for conencting to other switches and hosts for relaying frames.
Devices are "plugged into" this switch (and this switch can be plugged into other switches). 
When a frame is sent by the device on the other end of an interface, one of many receiving threads
processes the frame and places it in a thread-safe queue. A single sending thread then
takes the frame and sends it to the interface associated with the destination mac address
according to a routing table (which is built up via inspecting source mac addresses)
*/

using namespace std;

#define NUMBER_OF_INTERFACES 10      // the assumed number of physical interfaces the switch has 

// #define DEBUG

Switch::Switch() {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<MPDU*>;
    name = "unnamed";
    switch_print("Online");
}

Switch::Switch(std::string switch_name) {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<MPDU*>;
    name = switch_name;
    switch_print("Online");
}

Switch::Switch(std::vector<Host*> hosts_to_connect, std::string switch_name) {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<MPDU*>;
    name = switch_name;
    switch_print("Online");
}

Switch::~Switch() {
    for (int i = 0; i < switch_table.size(); i++) {
        delete switch_table.at(i);
        delete rx_interfaces.at(i);
        delete tx_interfaces.at(i);
    }
    delete frame_queue;
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

void Switch::plug_in_device(Switch* device_to_connect) {
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


void Switch::sender() {
    MPDU* tx_frame;    // pointer to the frame to be sent out
    int if_id;          // the id of the interface that is sending the frame

    // continually check queue for frames
    while(1) {
        // get the next frame
        tx_frame = frame_queue->remove();
        // find if this frame is for a device that has been registered in the table
        if_id = get_table_interface_number(tx_frame->get_destination_mac());
        if (if_id != -1) {
            // send to the one interface associated with the device
            unicast(tx_frame, if_id);
        } else {
            // send to all interfaces
#ifdef DEBUG
            switch_print("Cannot find host MAC in routing table: " + mac_to_string(tx_frame->get_destination_mac()));
#endif
            broadcast(tx_frame);
        }  
    }
}

void Switch::unicast(MPDU* tx_frame, int if_id) {
    // send one frame on a specific interface

#ifdef DEBUG
    switch_print("Transmitting frame on interface " + std::to_string(if_id));
#endif

    // will block until that interface is free
    tx_interfaces.at(if_id)->transmit(tx_frame);
}

void Switch::broadcast(MPDU* tx_frame) {
    // send frames on all interfaces except the in interface, in_if
#ifdef DEBUG
    switch_print("broadcasting");
#endif
    for (int i = 0; i < tx_interfaces.size(); i++) {
        // check if interface number, i, is in one of the TableEntry 
        // objects. if it is and the interface isn't the same as the in_interface
        // send on that interface. if it isn't in the table, send on that interface as well.
        // this follows from the fact that a table entry for this frame's sender interface
        // must have ben added and thus we send to all other interfaces that have been added.
        if (get_table_interface_number(tx_frame->get_source_mac()) != i) {
            unicast(tx_frame->copy(), i);
        }
    }
    delete tx_frame;
}

void Switch::receiver(int if_id) {
    MPDU* rx_frame;
    // continually loop in trying to obtain a new frame from the medium
    while (1) {
        rx_frame = rx_interfaces.at(if_id)->receive();
#ifdef DEBUG
        switch_print("Frame received on interface " + std::to_string(if_id));
#endif
        process_frame(rx_frame, if_id);
    }
}

void Switch::process_frame(MPDU* rx_frame, int in_if) {
    // add the frame to the queue of frames to be transmitted
    // check to see if the sending device is listed and add the device to the i/f table if its not there
    int if_id = get_table_interface_number(rx_frame->get_source_mac());

    if (if_id == -1) {
        // add interface to table if it does not yet exist there
        add_table_entry(rx_frame->get_source_mac(), in_if);
    }

    frame_queue->add(rx_frame);

}

int Switch::get_table_interface_number(std::vector<uint8_t> mac_addr) {
    // obtain the interface number associatd with this device.
    // of note is that an interface can be associated with many devices, something that will
    // have to be updated in the code when we move to a true network format
    int result = -1;

    if (switch_table.size() != 0 && !is_broadcast(mac_addr)) {
        for (int i = 0; i < switch_table.size(); i++) {
            for (std::vector<std::vector<uint8_t>>::iterator it = switch_table.at(i)->address_list.begin();
                it != switch_table.at(i)->address_list.end(); ++it){ 
                if (compare_macs(mac_addr,*it)) {
                    return (switch_table.at(i))->interface_number;
                }
            }
        }
    }

    return result;
}

void Switch::add_table_entry(std::vector<uint8_t> source_mac, int if_id) {
    // add a new entry to the routing table that associates an interface id
    // to the mac address

    // search existing table to see if this interface exists in the table
    // and add the mac to the address list for this interface if it does
    for (int i = 0; i < switch_table.size(); i++) {
        if (switch_table.at(i)->interface_number == if_id) {
            switch_table.at(i)->address_list.push_back(source_mac);
            return;
        }
    }

    // otherwise crease a new entry to the table
    TableEntry* new_entry = new TableEntry;
    new_entry->interface_number = if_id;
    new_entry->address_list.push_back(source_mac);
    switch_table.push_back(new_entry);
#ifdef DEBUG
    switch_print("Table entry added for " + mac_to_string(source_mac));
    print_routing_table();
#endif
}

void Switch::print_routing_table() {
    // show the current routining tabel for the switch as it has been learned
    // by inspecting source mac addresses
    if (switch_table.size() > 0) {
        switch_print("***************************");
        for (int i = 0; i < switch_table.size(); i++) {
            for (std::vector<std::vector<uint8_t>>::iterator it = switch_table.at(i)->address_list.begin();
                it != switch_table.at(i)->address_list.end(); ++it) {
                switch_print(std::to_string(switch_table.at(i)->interface_number) + " : " + 
                    mac_to_string(*it));
            }
        }
        switch_print("***************************");
    }
}

void Switch::switch_print(std::string statement) {
    std::cout << setw(15) << name << ": " << statement << endl;
}








Router::Router() {
    main_dhcp = new DHCPServer(frame_queue, create_ip(192,168,0,0), 24, create_ip(192,168,0,4));
}      

Router::Router(uint32_t subnet_ip, uint32_t netmask, uint32_t DNS_ip){
    main_dhcp = new DHCPServer(frame_queue, subnet_ip, netmask, DNS_ip);
}

std::string Router::formulate_ACK(){

}

void Router::receiver() {

}

void Router::sender(){

}









DHCPServer::DHCPServer(wqueue<MPDU*>* router_queue)  {
    tx_queue = router_queue;
    subnet_ip = create_ip(192,168,0,0);
    netmask = create_ip(255,255,255,0);
    DNS_server = create_ip(192,168,0,4);
}

DHCPServer::DHCPServer(wqueue<MPDU*>* router_queue, uint32_t IP, uint32_t mask) {
    tx_queue = router_queue;
    subnet_ip = IP;
    netmask = mask;
    DNS_server = create_ip(192,168,0,4);
}

DHCPServer::DHCPServer(wqueue<MPDU*>* router_queue, uint32_t IP, uint32_t mask, uint32_t dns) {
    tx_queue = router_queue;
    subnet_ip = IP;
    netmask = mask;
    DNS_server = dns;
}


void DHCPServer::set_subnet(uint32_t IP, uint32_t mask) {
    subnet_ip = IP;
    netmask = mask;
}

void DHCPServer::set_subnet_ip(uint32_t IP) {
    subnet_ip = IP;
}

void DHCPServer::set_subnet_netmask(uint32_t mask) {
    netmask = mask;
}

void DHCPServer::set_DNS_server(uint32_t dns){
    DNS_server = dns;
}

void DHCPServer::new_DHCP_discover(uint32_t transaction_id, std::vector<uint8_t> source_mac) {
    // for now, assume that the client is not already represented. I'll figure out how
    // to deal with the situation that a client requests an address when they already have
    // one in the DHCP table


    // for (std::vector<DHCPEntry>::iterator it = table.begin(); it != table.end(); ++it)
}

void DHCPServer::send_DHCP_offer() {
    // generate ip
    // create packet with transaction id and 
}




std::vector<uint8_t> DHCPServer::generate_ip() {};
void DHCPServer::flush_ips() {};




// int main() {
//     return 0;
// }
