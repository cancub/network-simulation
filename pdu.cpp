#include <string>
#include "pdu.h"
#include <iostream>
#include <cstdint>
#include <vector>
#include "l3_protocols.h"
#include "addressing.h"
#include <sstream>

/*
    MPDUs are the objects that will move around the network, being
    created by a source host and delated by the receiving host
*/

using namespace std;

Socket::Socket(uint16_t port_number, uint8_t protocol_number) {
    rx_queue = new wqueue<MPDU*>;
    port = port_number;
    protocol = protocol_number;
}

Socket::~Socket() {
    delete rx_queue;
}

MPDU* Socket::get_frame() {
    return rx_queue->remove();
}

void Socket::add_frame(MPDU* frame_to_add) {
    rx_queue->add(frame_to_add);
}

uint16_t Socket::get_port() {
    return port;
}

uint8_t Socket::get_protocol() {
    return protocol;
}




IP::IP(){
    header_length = 12; // header_length + total_length + protocol + source and dest IPs
}
IP::IP(ICMP){}
IP::IP(TCP){}
IP::IP(UDP){}

// take a u8 version of the IP frame and create an IP object from it
IP::IP(std::vector<uint8_t> ip_u8) {

    int i = 0;
    // uint8_t header_length;
    header_length = ip_u8[0];

    // uint16_t total_length;
    total_length = ((uint16_t)(ip_u8[1]) << 8) + ip_u8[2];

    // uint8_t protocol;
    protocol = ip_u8[3];

    // uint32_t source_ip;
    source_ip = create_ip(ip_u8[4],ip_u8[5],ip_u8[6],ip_u8[7]);

    // uint32_t destination_ip;
    destination_ip = create_ip(ip_u8[8],ip_u8[9],ip_u8[10],ip_u8[11]);

    // std::vector<uint8_t> SDU;
    SDU.reserve(total_length - header_length);
    for (int i = 0; i < total_length - header_length; i++) {
        SDU.push_back(ip_u8[i+12]);
    }
}


void IP::encap_SDU(ICMP new_ICMP) {
    // set the type since we know it's ARP

    protocol = IP_PROTOCOL_ICMP; // ICMP

    // cout << "header length set to " << std::to_string(header_length) << endl;

    SDU.clear();
    SDU.reserve(8 + new_ICMP.payload.size());

    int j = 0;

    SDU.push_back(new_ICMP.type);
    SDU.push_back(new_ICMP.code);

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ICMP.checksum >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ICMP.identifier >> (i*8)) & 0xFF);
    }

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ICMP.sequence_number >> (i*8)) & 0xFF);
    }

    for (int i = 0; i < new_ICMP.payload.size(); i++) {
        SDU.push_back(new_ICMP.payload[i]);
    }

    total_length = header_length + SDU.size();

}
void IP::encap_SDU(TCP) {}
void IP::encap_SDU(UDP) {}
IP::~IP() {}

void IP::set_source_ip(uint32_t input_source_ip) {source_ip = input_source_ip;}
void IP::set_destination_ip(uint32_t input_destination_ip) {destination_ip = input_destination_ip;}
uint8_t IP::get_header_length() {return header_length;}
uint16_t IP::get_total_length() {return total_length;}
uint32_t IP::get_source_ip() {return source_ip;}
uint32_t IP::get_destination_ip() {return destination_ip;}
uint8_t IP::get_protocol() {return protocol;}
uint16_t IP::get_SDU_length() {return SDU.size();}
std::vector<uint8_t> IP::get_SDU() {return SDU;}

IP generate_IP(std::vector<uint8_t> ip_u8) {

    IP to_return(ip_u8);

    return to_return;
}



MPDU::MPDU() {
    source_mac.reserve(6);
    destination_mac.reserve(6);
    SDU.reserve(9);
}

MPDU::~MPDU() {}

MPDU* MPDU::copy() {
    // sometimes a MPDU needs to be sent out multiple interfaces, so it will need to be copied
    return new MPDU(*this);
}

std::vector<uint8_t> MPDU::get_source_mac() { return source_mac; }

std::vector<uint8_t> MPDU::get_destination_mac() { return destination_mac; }
        
size_t MPDU::get_size() {return 12 + SDU_length;}

void MPDU::set_source_mac(std::vector<uint8_t> mac) {
    source_mac = mac;
}

void MPDU::set_destination_mac(std::vector<uint8_t> mac) {
    destination_mac = mac;
}

void MPDU::encap_SDU(IP new_IP) {
    uint8_t * temp_ip;

    // set the type since we know it's IP
    SDU_type = 0x0800;

    SDU_length = new_IP.get_total_length();

    // and since we know it's IP, we know what the headers are and how much space they
    // take up, so we can copy the contents, byte by byte into the SDU (since the 
    // data link layer should be agnostic to the L2 frame payload contents)

    SDU.reserve(new_IP.get_total_length());

    // header length 1 byte
    SDU.push_back(new_IP.get_header_length());

    // total length 2 bytes
    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_IP.get_total_length() >> (i*8)) & 0xFF);
    }

    // protocol 1 byte
    SDU.push_back(new_IP.get_protocol());

    // source ip 4 bytes
    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_IP.get_source_ip() >> (i*8)) & 0xFF);
    }

    // dest ip 4 bytes
    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_IP.get_destination_ip() >> (i*8)) & 0xFF);
    }

    // SDU

    std::vector<uint8_t> SDU_to_add = new_IP.get_SDU();

    for (int i = 0; i < new_IP.get_SDU_length(); i++) {
        SDU.push_back(SDU_to_add[i]);
    }
}

void MPDU::encap_SDU(ARP new_ARP) {
    // set the type since we know it's ARP
    SDU_type = 0x0806;

    SDU_length = 22;
    SDU.clear();
    SDU.reserve(SDU_length);

    int j = 0;

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ARP.opcode >> (i*8)) & 0xFF);
    }

    for (int i = 0; i < 6; i++) {
        SDU.push_back(new_ARP.sender_mac[i]);
    }

    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_ARP.sender_ip >> (i*8)) & 0xFF);
    }

    for (int i = 0; i < 6; i++) {
        SDU.push_back(new_ARP.target_mac[i]);
    }

    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_ARP.target_ip >> (i*8)) & 0xFF);
    }
}

uint16_t MPDU::get_SDU_type() {return SDU_type;}

std::vector<uint8_t> MPDU::get_SDU() {return SDU;}

void MPDU::erase() {
    source_mac.clear();
    source_mac.reserve(6);
    destination_mac.clear();
    destination_mac.reserve(6);
    SDU.clear();
    SDU.reserve(9);

}

void print_bytes(void* to_print, int number_of_bytes) {
    uint8_t* printer = (uint8_t*)to_print;
    for (int i = 0; i < number_of_bytes; i++) {
        if (printer[i] < 16) {
            cout << "0";
        }
        cout << std::hex << (int)(printer[i]) << " ";
    }
    cout << endl;
}

// void print_bytes(void* to_print, int number_of_bytes) {
//     uint8_t* printer = (uint8_t*)to_print;
//     for (int i = 0; i < number_of_bytes; i++) {
//         if (printer[i] < 16) {
//             cout << "0";
//         }
//         cout << std::hex << (int)(printer[i]) << " ";
//     }
//     cout << endl;
// }


// int main() {

//     // create an ARP
//     ARP my_arp;
//     my_arp.opcode = 0x0001;
//     my_arp.sender_mac = create_uniform_mac(0x11);
//     my_arp.sender_ip = create_ip(192,168,0,10);
//     my_arp.target_mac = create_uniform_mac(0);
//     my_arp.target_ip = create_ip(192,168,0,30);

//     // encap it

//     MPDU my_mpdu;
//     my_mpdu.set_source_mac(my_arp.sender_mac);
//     my_mpdu.set_destination_mac(create_broadcast_mac());
//     my_mpdu.encap_SDU(my_arp);


//     uint16_t my_SDU_type = my_mpdu.get_SDU_type();

//     // print out contets
//     print_bytes((void*)(&my_SDU_type), 2);

//     std::vector<uint8_t> my_SDU = my_mpdu.get_SDU();

//     print_bytes((void*)(&(my_SDU[0])), 22);

//     ARP second_arp = generate_ARP(my_SDU);

//     my_mpdu.encap_SDU(second_arp);

//     my_SDU = my_mpdu.get_SDU();

//     print_bytes((void*)(&(my_SDU[0])), 22);

// }

// int main() {

//     // create an icmp
//     ICMP my_icmp;
//     my_icmp.type = 0;
//     my_icmp.code = 1;
//     my_icmp.checksum = 0xeeb7;
//     my_icmp.sequence_number = 20;
//     my_icmp.payload.reserve(10);
//     for (int i = 0; i < 10; i++){
//         my_icmp.payload.push_back(i);
//     }

//     cout << to_string(my_icmp.type) << endl;
//     cout << to_string(my_icmp.code) << endl;
//     cout << my_icmp.checksum << endl;
//     cout << my_icmp.sequence_number << endl;
//     for(int i = 0; i < my_icmp.payload.size(); i++) {
//         cout << to_string(my_icmp.payload[i]) << " ";
//     }
//     cout << endl;


//     // encap it in IP
//     IP my_ip;
//     uint32_t source_ip = create_random_ip();
//     my_ip.set_source_ip(source_ip);
//     cout << ip_to_string(my_ip.get_source_ip()) << endl;

//     uint32_t destination_ip = create_random_ip();
//     my_ip.set_destination_ip(destination_ip);
//     cout << ip_to_string(my_ip.get_destination_ip()) << endl;
//     my_ip.encap_SDU(my_icmp);

//     MPDU my_mpdu;
//     my_mpdu.set_source_mac(create_random_mac());
//     my_mpdu.set_destination_mac(create_broadcast_mac());
//     my_mpdu.encap_SDU(my_ip); 


//     IP second_ip(my_mpdu.get_SDU());

//     cout << ip_to_string(second_ip.get_source_ip()) << endl;
//     cout << ip_to_string(second_ip.get_destination_ip()) << endl;


//     // obtain the SDU and get a new ICMP from it
//     std::vector<uint8_t> my_SDU = second_ip.get_SDU();
//     ICMP second_icmp = generate_ICMP(my_SDU);

//     // print out the results
//     cout << to_string(second_icmp.type) << endl;
//     cout << to_string(second_icmp.code) << endl;
//     cout << second_icmp.checksum << endl;
//     cout << second_icmp.sequence_number << endl;
//     for(int i = 0; i < second_icmp.payload.size(); i++) {
//         cout << to_string(second_icmp.payload[i]) << " ";
//     }
//     cout << endl;

// }