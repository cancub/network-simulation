#include <string>
#include "pdu.h"
#include <iostream>
#include <cstdint>
#include <vector>
#include "l3_protocols.h"
#include "l4_protocols.h"
#include "addressing.h"
#include <sstream>
#include <fstream>

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
        
size_t MPDU::get_size() {return MPDU_HEADER_SIZE + SDU.size();}

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



    // and since we know it's IP, we know what the headers are and how much space they
    // take up, so we can copy the contents, byte by byte into the SDU (since the 
    // data link layer should be agnostic to the L2 frame payload contents)
    SDU.clear();
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

    SDU.clear();
    SDU.reserve(22); // standard ARP size for this simulator

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

//     uint32_t my_ip = create_ip(1,1,1,1);
//     uint32_t dest_ip = create_ip(2,2,2,2);

//     // make a TCP frame, load up values
//     TCP test_segment;
//     test_segment.source_port = 30000;
//     test_segment.destination_port = 50000;
//     test_segment.sequence_number = 51231;
//     test_segment.ACK_number = 12341;
//     test_segment.details = 0x01; //FIN
//     test_segment.receive_window = 20;

//     // open the test text file and load it into the segment
//     std::ifstream ifs;
//     ifs.open ("test.txt", ifstream::in);
//     char c = ifs.get();

//     while (ifs.good()) {
//         test_segment.payload.push_back((uint8_t)c);

//         c = ifs.get();
//     }

//     ifs.close();

//     // encap in IP

//     IP first_ip;
//     first_ip.set_destination_ip(dest_ip);
//     first_ip.set_source_ip(my_ip);
//     first_ip.encap_SDU(test_segment);

//     // encap in MPDU

//     MPDU test_mpdu;
//     test_mpdu.encap_SDU(first_ip);

//     // de-encap in IP
//     IP second_ip = generate_IP(test_mpdu.get_SDU());

//     // de-encap in TCP
//     TCP second_tcp = generate_TCP(second_ip.get_SDU());



//     cout << second_tcp.source_port << endl;
//     cout << second_tcp.destination_port << endl;
//     cout << second_tcp.sequence_number << endl;
//     cout << second_tcp.ACK_number << endl;
//     cout << second_tcp.details << endl;
//     cout << second_tcp.receive_window << endl;

//     std::ofstream ofs;
//     ofs.open("test_1.txt", ofstream::out);

//     for (int i = 0; i < second_tcp.payload.size(); ++i) {
//         ofs.put(second_tcp.payload[i]);
//     }

//     ofs.close();


//     // TCP_sender * my_sender = new TCP_sender(my_ip, my_port, dest_ip, dest_port);


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