#ifndef PDU_H
#define PDU_H

#include <stdlib.h>
#include <string>
#include <cstdint>
#include <vector>
#include "l3_protocols.h"

/*
This is the class that will be our interface, since what is an interface if not
just a pointer to a MPDU on a link?
*/

using namespace std;


class TCP {
    public:
        TCP(); // default constructor
        ~TCP();
        size_t get_size();
        void erase();   // reset the MPDU to be blank
        TCP* copy();
    private:
        size_t SDU_length;
        std::vector<uint8_t> SDU;
};

struct UDP {
    size_t SDU_length;
    std::vector<uint8_t> SDU;
};

class IP { 
    public:
        IP();
        IP(ICMP);
        IP(TCP);
        IP(UDP);
        void encap_SDU(ICMP);
        void encap_SDU(TCP);
        void encap_SDU(UDP);
        void set_src_ip(uint32_t);
        void set_dest_ip(uint32_t);
        uint32_t get_source_ip();
        uint32_t get_destination_ip();
        uint8_t get_protocol();
        uint16_t get_SDU_length();
        uint8_t get_header_length();
        uint16_t get_total_length();
        std::vector<uint8_t> get_SDU();
        ~IP();
    private:     
        uint8_t header_length;
        uint16_t total_length;
        uint32_t source_ip;
        uint32_t destination_ip;
        uint8_t protocol;
        uint16_t SDU_type;
        uint16_t SDU_length;

        std::vector<uint8_t> SDU;
};


class MPDU {
    public:
        MPDU(); // default constructor
        ~MPDU();
        std::vector<uint8_t> get_src_mac();
        std::vector<uint8_t> get_dst_mac();
        MPDU* copy();
        size_t get_size();
        void set_src_mac(std::vector<uint8_t>);
        void set_dst_mac(std::vector<uint8_t>);
        void encap_SDU(IP);        
        void encap_SDU(ARP);
        uint16_t get_SDU_type();
        std::vector<uint8_t> get_SDU();
        void erase();   // reset the MPDU to be blank
    private:
        std::vector<uint8_t> source_mac;
        std::vector<uint8_t> destination_mac;
        size_t SDU_length;
        uint16_t SDU_type;
        std::vector<uint8_t> SDU;
};


#endif