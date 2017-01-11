#ifndef DATA_LINKS_H
#define DATA_LINKS_H

#include <mutex>
#include <vector>
#include <cstdint>
#include <stdlib.h>
#include "pdu.h"
#include <condition_variable> // condition_variable
#include "wqueue.h"

using namespace std;

/*
These are the links that will be used to connect nodes in the network.
There will likely only be two links: Ethernet and air.
Ethernet should be straight forward in that it's a passive object
that just has an interface that nodes can add a frame to
or retrieve a frame from, and there is a mutex that must be locked
to interact with this interface. Air will be a bit different as there
should be delay depending on the location of the station
and there should also be a possibility of collision on the link.

Ethernet should be agnostic to who or what is trying to interact with it.
It should simply be a well-described object that both ends of the link
can actively engage while it just sits there.
*/

#define FAILURES_PER_1000000
#define CORRUPTIONS_PER_1000000





// the wire is what actuallt carries the information

class EthernetWire {
public:    
    EthernetWire( chrono::time_point<chrono::high_resolution_clock> start_time, bool monitor, string name, int * fc_ptr);
    ~EthernetWire();
    void transmit(MPDU*);
    MPDU* receive();
private:
    wqueue<MPDU*>* interface;
    chrono::time_point<chrono::high_resolution_clock> link_start_time;
    bool print_metadata;
    string if_name;
    int * frame_count_ptr;
};

// a link is comprised of two wires for bi-directional traffic

class EthernetLink {
    
    public:
        EthernetLink(bool monitor_interface = false, string name = "");
        ~EthernetLink();
        EthernetWire* get_wire_1(){return FD_wire_1;};
        EthernetWire* get_wire_2(){return FD_wire_2;};
    private:
        EthernetWire* FD_wire_1; // ethernet cables are full duplex
        EthernetWire* FD_wire_2;
        bool print_metadata;
        int frame_count; // keep track of number of frames received
};

class Air {};

#endif