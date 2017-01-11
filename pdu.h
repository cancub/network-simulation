#ifndef PDU_H
#define PDU_H

#include <stdlib.h>
#include <string>
#include <cstdint>
#include <vector>
#include "l3_protocols.h"
#include "wqueue.h"

using namespace std;

#define MPDU_ARP_TYPE 0x0806
#define MPDU_IP_TYPE  0x0800


#define MPDU_HEADER_SIZE    12  


class MPDU {
    public:
        MPDU(); // default constructor
        ~MPDU();
        std::vector<uint8_t> get_source_mac();
        std::vector<uint8_t> get_destination_mac();
        MPDU* copy();
        size_t get_size();
        void set_source_mac(std::vector<uint8_t>);
        void set_destination_mac(std::vector<uint8_t>);
        void encap_SDU(IP);        
        void encap_SDU(ARP);
        uint16_t get_SDU_type();
        std::vector<uint8_t> get_SDU();
        void erase();   // reset the MPDU to be blank
    private:
        std::vector<uint8_t> source_mac;
        std::vector<uint8_t> destination_mac;
        uint16_t SDU_type;
        std::vector<uint8_t> SDU;
};

class Socket {
    public:
        Socket(uint16_t, uint8_t);
        ~Socket();
        MPDU* get_frame(){return rx_queue->remove();}
        void add_frame(MPDU* frame_to_add) {rx_queue->add(frame_to_add);}
        uint16_t get_port() {return port;}
        uint8_t get_protocol() {return protocol;}
        size_t queue_size() {return rx_queue->size();}
    private:
        uint16_t port;
        uint8_t protocol;
        wqueue<MPDU*>* rx_queue;
};


#endif