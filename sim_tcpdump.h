#ifndef SIM_TCPDUMP_H
#define SIM_TCPDUMP_H

#include "pdu.h"
#include "l3_protocols.h"
#include "l4_protocols.h"
#include <cstdint>
#include <stdlib.h>

using namespace std;

struct frame_details {
    string protocol_type;
    vector<string> info;
};

string * get_frame_details(MPDU* tx_frame);

#endif