#include <mutex>
#include <iostream>
#include "data_links.h"
#include "pdu.h"
#include <condition_variable> // condition_variable
#include "wqueue.h"
#include <stdlib.h>
#include "misc.h"
#include "addressing.h"
#include "sim_tcpdump.h"
#include <chrono>

using namespace std;


EthernetWire::EthernetWire(chrono::time_point<chrono::high_resolution_clock> start_time, bool monitor, string name,
    int* fc_ptr) {
    //initialize a queue as the interface and set the maximum size of the queue to however
    // many frames this interface can hold (in this case 1)
    interface = new wqueue<MPDU*>("",false);
    interface->set_max_size(1);
    link_start_time = start_time;
    print_metadata = monitor;
    frame_count_ptr = fc_ptr;
    if_name = name;
}

EthernetWire::~EthernetWire() {
    delete interface;
}

void EthernetWire::transmit(MPDU* tx_frame) {
    // the method for transmitting information across the link as well as showing tcpdump/wireshark-esque info



    if (print_metadata) {
        chrono::duration<double> diff = chrono::high_resolution_clock::now() - link_start_time;
        (*frame_count_ptr)++;

        string * frame_info = get_frame_details(tx_frame);

        // cout << *frame_info << endl;
            
        string statement = "[" + if_name + "] " + to_string((*frame_count_ptr)) + " " + to_string(diff.count()) 
                                + " " + *frame_info;

        cout << statement << endl;


        // for (vector<string>::iterator it = frame_info->info.begin(); it != frame_info->info.end(); ++it) {
        //     cout << statement << endl;
            
        //     statement += " " + *it;
        // }

        delete frame_info;
    }

    interface->add(tx_frame);
    // print the information about the 
    // in the case a frame being dropped, just get rid of the frame
    /*if (rand() % 1000000 <= FAILURES_PER_1000000) {
        delete tx_frame;
    } else if (rand() % 1000000 <= CORRUPTIONS_PER_1000000) {
        // produce a corruption then transmit
        corrupt_frame(tx_frame);
        interface->add(tx_frame);
    } else {
        // transmit the frame just fine
    }*/
}

MPDU* EthernetWire::receive() {
    // wait for a signal to be received and then process the frames
    return interface->remove();
}




// the EthernetLink is used to set up the wires and provide them to the device to which they are being connected

EthernetLink::EthernetLink(bool monitor_interface, string name) {
    // monitor interface is used to determine if this is a link that should print out metadata about frames
    // that traverse it, somewhat like tcpdump and wireshark
    chrono::time_point<chrono::high_resolution_clock> link_start_time = chrono::high_resolution_clock::now();
    frame_count = 0;
    FD_wire_1 = new EthernetWire(link_start_time, monitor_interface, name, &frame_count);
    FD_wire_2 = new EthernetWire(link_start_time, monitor_interface, name, &frame_count);
}

EthernetLink::~EthernetLink() {
    delete FD_wire_1;
    delete FD_wire_2;
}

