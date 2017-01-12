#include <string>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <condition_variable> // condition_variable
#include <iomanip>
#include "unistd.h"
#include "networking_devices.h"
#include "host.h"
#include "pdu.h"
#include "data_links.h"
#include "wqueue.h"
#include "addressing.h"
#include "l4_protocols.h"
#include "l3_protocols.h"

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

DHCP generate_DHCP(vector<uint8_t> dhcp_u8) {
    DHCP result;
    result.opcode = dhcp_u8[0];
    result.client_ip = (dhcp_u8[1] << 24) + (dhcp_u8[2] << 16) + (dhcp_u8[3] << 8) + (dhcp_u8[4]);
    result.your_ip = (dhcp_u8[5] << 24) + (dhcp_u8[6] << 16) + (dhcp_u8[7] << 8) + (dhcp_u8[8]);

    return result;
}

DHCPServer::DHCPServer(wqueue<MPDU*>* input_if, EthernetWire* output_if, vector<uint8_t> mac, uint32_t ip) {
    input_queue = input_if;
    tx_if = output_if;
    gw_mac = mac;
    gw_ip = ip;
}

void DHCPServer::run() {
    MPDU *tx_frame, *rx_frame;
    IP packet;
    UDP segment;
    DHCP message;
    DHCPEntry entry;

    // process any and all DHCP frames and send responses
    while (true) {

        // get a frame and find the DHCP message
        rx_frame = input_queue->remove();
        // find the entry associated with this MAC address
        entry = get_entry(rx_frame->get_source_mac());
        packet = generate_IP(rx_frame->get_SDU());
        segment = generate_UDP(packet.get_SDU());
        message = generate_DHCP(segment.payload);

        switch(message.opcode) {
            case DHCP_DISCOVER:
                if (entry.current_opcode == DHCP_NO_ENTRY) {
                    // add a new entry if the client does not yet exist in the table
                    entry.mac = rx_frame->get_source_mac();
                    // for the purposes of testing, we need deterministic IPs so that subsequent steps will work
                    // NOTE: later on, give out IPs in order for subnet so that hosts can just figure out
                    // the other host's IPs based on their own IP
                    if (compare_macs(create_uniform_mac(0x11), entry.mac)) {
                        entry.ip = create_ip(111,111,111,111);
                    } else if (compare_macs(create_uniform_mac(0x22), entry.mac)) {
                        entry.ip = create_ip(222,222,222,222);
                    }  else {
                        cout << "unexpected MAC" << endl;
                    }
                    
                    entry.current_opcode = DHCP_OFFER;
                    table.push_back(entry);
                } 

                // send out a frame to let the client know its IP offer
                message.opcode = DHCP_OFFER;
                message.your_ip = entry.ip;
                segment.source_port = BOOTP_SERVER_PORT;
                segment.destination_port = BOOTP_CLIENT_PORT;
                segment.payload.clear();
                segment.payload.push_back(message.opcode);
                for (int i = 3; i >= 0; i--) {
                    segment.payload.push_back((message.client_ip >> (i*8)) & 0xFF);
                }
                for (int i = 3; i >= 0; i--) {
                    segment.payload.push_back((message.your_ip >> (i*8)) & 0xFF);
                }

                packet.set_source_ip(gw_ip);
                packet.set_destination_ip(create_broadcast_ip());
                packet.encap_SDU(segment);

                // encap in MPDU
                tx_frame = new MPDU();
                tx_frame->set_source_mac(gw_mac);
                tx_frame->set_destination_mac(entry.mac);
                tx_frame->encap_SDU(packet);


                // send frame

                tx_if->transmit(tx_frame);

                break;
            case DHCP_REQUEST:
                // send out a frame to let the client know it can take the IP if the IPs match
                if (message.client_ip == entry.ip) {
                    message.opcode = DHCP_ACK;
                } else {
                    // send a NACK, this is not the IP that was offered by the server
                    message.opcode = DHCP_NACK;
                    message.your_ip = entry.ip;
                }

                segment.source_port = BOOTP_SERVER_PORT;
                segment.destination_port = BOOTP_CLIENT_PORT;
                segment.payload.clear();
                segment.payload.push_back(message.opcode);
                for (int i = 3; i >= 0; i--) {
                    segment.payload.push_back((message.client_ip >> (i*8)) & 0xFF);
                }
                for (int i = 3; i >= 0; i--) {
                    segment.payload.push_back((message.your_ip >> (i*8)) & 0xFF);
                }

                // packet.set_source_ip(gw_ip);
                // packet.set_destination_ip(create_broadcast_ip());
                packet.encap_SDU(segment);

                // encap in MPDU
                tx_frame = new MPDU();
                tx_frame->set_source_mac(gw_mac);
                tx_frame->set_destination_mac(entry.mac);
                tx_frame->encap_SDU(packet);

                // send frame

                tx_if->transmit(tx_frame);
                break;
            default:
                cout << to_string(message.opcode) << endl;
        }
        delete rx_frame;
    }
}

void DHCPServer::set_subnet(uint32_t, uint32_t) {}
void DHCPServer::set_subnet_ip(uint32_t) {}
void DHCPServer::set_subnet_netmask(uint32_t) {}

DHCPEntry DHCPServer::get_entry(vector<uint8_t> test_mac) {
    DHCPEntry result;
    result.current_opcode = DHCP_NO_ENTRY;
    for (vector<DHCPEntry>::iterator it = table.begin(); it != table.end(); ++it) {
        if (compare_macs(test_mac, it->mac)) {
            result = *it;
        }
    }
    return result;
}

void DHCPServer::set_entry(vector<uint8_t> test_mac, uint32_t set_ip, uint8_t opcode) {
    for (vector<DHCPEntry>::iterator it = table.begin(); it != table.end(); ++it) {
        if (compare_macs(test_mac, it->mac)) {
            it->ip = set_ip;
            it->current_opcode = opcode;
            break;
        }
    }
}



void DHCPServer::flush_ips() {}



Switch::Switch() {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<MPDU*>;
    name = "unnamed";
    switch_print("Online");
}

Switch::Switch(string switch_name, mutex* sw_mutex) {
    rx_interfaces.reserve(NUMBER_OF_INTERFACES);
    tx_interfaces.reserve(NUMBER_OF_INTERFACES);
    switch_table.reserve(NUMBER_OF_INTERFACES);
    rx_thread_list.reserve(NUMBER_OF_INTERFACES);
    frame_queue = new wqueue<MPDU*>;
    name = switch_name;
    mutex_m = sw_mutex;
    switch_print("Online");
}

Switch::Switch(vector<Host*> hosts_to_connect, string switch_name) {
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
    }
    delete frame_queue;
}

void Switch::connect_ethernet(EthernetLink* e_link, bool flip_wires = false) {
    // the reason for flipping the wires involved in this link is that if we were to call the same
    // function on both sides of the link without flipping the wires, both ends would be using
    // the same link for transmissions and the same link for receptions, meaning nothing would get through
    if (!flip_wires) {
        tx_interfaces.push_back(e_link->get_wire_1());
        rx_interfaces.push_back(e_link->get_wire_2());
    } else {
        rx_interfaces.push_back(e_link->get_wire_1());
        tx_interfaces.push_back(e_link->get_wire_2());
    }
}

void Switch::run() {
    // start up sending thread with this switch so that the thread can access the queue and
    // tx interfaces
    thread tx_thread(&Switch::sender, this);

    // start up receiving threads which can also view the queue and rx interfaces, specifying
    // which interface each thread should be concerned with
    for (int i = 0; i < rx_interfaces.size(); i++) {
        thread* rx_thread = new thread(&Switch::receiver, this, i);
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
    switch_print("Transmitting frame on interface " + to_string(if_id));
#endif
    usleep((rand() % 1000) + 100);
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
        switch_print("Frame received on interface " + to_string(if_id));
#endif
        usleep((rand() % 1000) + 100);
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

int Switch::get_table_interface_number(vector<uint8_t> mac_addr) {
    // obtain the interface number associatd with this device.
    // of note is that an interface can be associated with many devices, something that will
    // have to be updated in the code when we move to a true network format
    int result = -1;

    if (switch_table.size() != 0 && !is_broadcast(mac_addr)) {
        for (int i = 0; i < switch_table.size(); i++) {
            for (vector<vector<uint8_t>>::iterator it = switch_table.at(i)->address_list.begin();
                it != switch_table.at(i)->address_list.end(); ++it){ 
                if (compare_macs(mac_addr,*it)) {
                    return (switch_table.at(i))->interface_number;
                }
            }
        }
    }

    return result;
}

void Switch::add_table_entry(vector<uint8_t> source_mac, int if_id) {
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
            for (vector<vector<uint8_t>>::iterator it = switch_table.at(i)->address_list.begin();
                it != switch_table.at(i)->address_list.end(); ++it) {
                switch_print(to_string(switch_table.at(i)->interface_number) + " : " + 
                    mac_to_string(*it));
            }
        }
        switch_print("***************************");
    }
}

void Switch::switch_print(string statement) {
    cout << setw(15) << name << ": " << statement << endl;
}





Gateway::Gateway(string gw_name, mutex* gw_mutex, vector<uint8_t> gw_mac , uint32_t gw_ip) {
    // the gateway needs a connection to both the the internet and the LAN, so it needs
    // queues in both directions
    // internet_rx_if = internet_rx_queue;
    // internet_tx_if = internet_tx_queue;
    // lan_rx_if = lan_rx_queue;
    // lan_tx_if = lan_tx_queue;
    name = gw_name;
    mutex_m = gw_mutex;
    mac = gw_mac;
    ip = gw_ip;
    process_queue = new wqueue<MPDU*>;
    DHCP_queue = new wqueue<MPDU*>;
    gateway_print("online");
} 

Gateway::~Gateway() {
    delete process_queue;
    delete DHCP_queue;
}

void Gateway::connect_ethernet(EthernetLink* e_link, bool flip_wires) {
    // the reason for flipping the wires involved in this link is that if we were to call the same
    // function on both sides of the link without flipping the wires, both ends would be using
    // the same link for transmissions and the same link for receptions, meaning nothing would get through
    if (!flip_wires) {
        lan_tx_if = e_link->get_wire_1();
        lan_rx_if = e_link->get_wire_2();
    } else {
        lan_rx_if = e_link->get_wire_1();
        lan_tx_if = e_link->get_wire_2();
    }
}

void Gateway::run() {
    MPDU* rx_frame, *tx_frame;
    IP rx_packet;
    UDP rx_segment;
    DHCP rx_dhcp;

    uint8_t source_port, destination_port;
    vector<uint8_t> SDU;

    // start up the receiver thread
    main_dhcp = new DHCPServer(DHCP_queue, lan_tx_if, mac,ip); // the DHCP needs to be fed in frames and to have an outlet
    thread dhcp_thread(&DHCPServer::run,main_dhcp);
    
    thread receiver_thread(&Gateway::receiver,this);

    // the main part of the gateway is just looking at frames and determining where they should go
    while (true) {
        // obtain a frame from the process queue
        rx_frame = process_queue->remove();

        // at the moment, there is only going to be one type of frame that we care about, and that's 
        // DHCP. This can be determined by knowing that BOOTP clients use port 68, BOOTP servers use 67
        // so we can assume that this is a UDP segment and that the ports match. If we see this match
        // then we can pass this frame to the DHCP server
        rx_packet = generate_IP(rx_frame->get_SDU());
        rx_segment = generate_UDP(rx_packet.get_SDU());

        if (rx_segment.source_port == BOOTP_CLIENT_PORT && rx_segment.destination_port == BOOTP_SERVER_PORT) {
            DHCP_queue->add(rx_frame);
        } else {
            // we don't have anything else to do with this frame at the moment
            delete rx_frame;
        }
    }

    receiver_thread.join();

    dhcp_thread.join();


}

void Gateway::receiver() {
    // sometimes the processing of frames is delayed, so it is better to just remove the
    // frame from the link and add it to the queue so that the link is cleared and the
    // frame can be processed when the host has available resources

    MPDU* frame;

    // this is to be run in a thread that will wait on a condition variable to check the rx
    // interface.
    while (true) {
        frame = lan_rx_if->receive();
        usleep((rand() % 1000) + 100);
        process_queue->add(frame);
    }
}

void Gateway::gateway_print(string statement) {
    cout << setw(15) << name << ": " << statement << endl;
}





// int main() {
//     return 0;
// }
