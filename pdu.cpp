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

std::vector<uint8_t> MPDU::get_src_mac() { return source_mac; }

std::vector<uint8_t> MPDU::get_dst_mac() { return destination_mac; }
        
size_t MPDU::get_size() {return 12 + SDU_size;}

void MPDU::set_src_mac(std::vector<uint8_t> mac) {
    source_mac = mac;
}

void MPDU::set_dst_mac(std::vector<uint8_t> mac) {
    destination_mac = mac;
}

void MPDU::encap_SDU(IP new_SDU) {
    uint8_t * temp_ip;

    // set the type since we know it's IP
    SDU_type = 0x0800;

    // and since we know it's IP, we know what the headers are and how much space they
    // take up, so we can copy the contents, byte by byte into the SDU (since the 
    // data link layer should be agnostic to the L2 frame payload contents)

    SDU.reserve(new_SDU.total_length);

    for (int i = 0; i < 2; i++) {
        SDU.push_back(((uint8_t*)(new_SDU.total_length))[i]);
    }

    temp_ip = (uint8_t*)(new_SDU.source_ip);

    for (int i = 0; i < 4; i++) {
        SDU.push_back(temp_ip[i]);
    }

    temp_ip = (uint8_t*)(new_SDU.destination_ip);

    for (int i = 0; i < 4; i++) {
        SDU.push_back(temp_ip[i]);
    }

    SDU.push_back(new_SDU.protocol);

    for (int i = 0; i < SDU_size-11; i++) {
        SDU.push_back(new_SDU.SDU[i]);
    }
}

void MPDU::encap_SDU(ARP new_ARP) {
    // set the type since we know it's ARP
    SDU_type = 0x0806;

    SDU_size = 22;
    SDU.clear();
    SDU.reserve(SDU_size);

    int j = 0;

    for (int i = 1; i >= 0; i--) {
        SDU.push_back((new_ARP.opcode >> (i*8)) & 0xFF);
    }

    // for (std::vector<uint8_t>::iterator it = SDU.begin(); it != SDU.end(); ++it) {
    //     if (*it < 16) {
    //         cout << "0";
    //     }
    //     cout << std::hex << (int)(*it) << " "; 
    // }

    // cout << endl;

    for (int i = 0; i < 6; i++) {
        SDU.push_back(new_ARP.sender_mac[i]);
    }

    // for (std::vector<uint8_t>::iterator it = SDU.begin(); it != SDU.end(); ++it) {
    //     if (*it < 16) {
    //         cout << "0";
    //     }
    //     cout << std::hex << (int)(*it) << " "; 
    // }

    // cout << endl;

    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_ARP.sender_ip >> (i*8)) & 0xFF);
    }

    // for (std::vector<uint8_t>::iterator it = SDU.begin(); it != SDU.end(); ++it) {
    //     if (*it < 16) {
    //         cout << "0";
    //     }
    //     cout << std::hex << (int)(*it) << " "; 
    // }

    // cout << endl;

    for (int i = 0; i < 6; i++) {
        SDU.push_back(new_ARP.target_mac[i]);
    }

    // for (std::vector<uint8_t>::iterator it = SDU.begin(); it != SDU.end(); ++it) {
    //     if (*it < 16) {
    //         cout << "0";
    //     }
    //     cout << std::hex << (int)(*it) << " "; 
    // }
    // cout << endl;

    for (int i = 3; i >= 0; i--) {
        SDU.push_back((new_ARP.target_ip >> (i*8)) & 0xFF);
    }

    // for (std::vector<uint8_t>::iterator it = SDU.begin(); it != SDU.end(); ++it) {
    //     if (*it < 16) {
    //         cout << "0";
    //     }
    //     cout << std::hex << (int)(*it) << " "; 
    // }
    // cout << endl;
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
//     my_mpdu.set_src_mac(my_arp.sender_mac);
//     my_mpdu.set_dst_mac(create_broadcast_mac());
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