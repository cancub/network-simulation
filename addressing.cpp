#include <stdlib.h>
#include <iostream>
#include <vector>
#include "addressing.h"
#include <time.h>
#include <cstdint>
#include <sstream>

using namespace std;

std::vector<uint8_t> create_mac(uint8_t b1,uint8_t b2,uint8_t b3,uint8_t b4,uint8_t b5,uint8_t b6) {
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 

    result.push_back(b1);
    result.push_back(b2);
    result.push_back(b3);
    result.push_back(b4);
    result.push_back(b5);
    result.push_back(b6);

    return result;
}

std::vector<uint8_t> create_random_mac() {
    // as one might expect, this is a function
    // that takes no argument and generates
    // a random MAC address 
    
    int character_number;
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 

    for(int i = 0; i < 6; i++) {
        result.push_back((uint8_t)(rand() % 256));
    }

    return result;
}  

std::vector<uint8_t> create_uniform_mac(uint8_t mac_byte) {
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 
    for (int i = 0; i < 6; i++) {
        result.push_back(mac_byte);
    }
    return result;
}

std::vector<uint8_t> create_broadcast_mac() {
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 
    for (int i = 0; i < 6; i++) {
        result.push_back(0xFF);
    }
    return result;
}




uint32_t create_ip(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
    // again, as one might expect, this takes no argument
    // and returns a random ip

    uint32_t result = 0x00000000; // this will only be four bytes

    result |= b1;
    result = result << 8;
    result |= b2;
    result = result << 8;
    result |= b3;
    result = result << 8;
    result |= b4;

    return result;
}

uint32_t create_random_ip() {
    // again, as one might expect, this takes no argument
    // and returns a random ip

    uint32_t result = 0x00000000; // this will only be four bytes

    for(int i = 0; i < 4; i++) {
        result |= ((uint32_t) (rand() % 256)) << (i*8);
    }

    return result;
}

uint32_t create_broadcast_ip() {
    return 0xFFFFFFFF;
}

int compare_macs(std::vector<uint8_t> mac1, std::vector<uint8_t> mac2) {
    int result = 1;
    for (int i = 0; i < 6; i++){
        if (mac1[i] != mac2[i]){
            return 0;
        }
    }
    return result;
}

int compare_ips(uint32_t ip1, uint32_t ip2) {
    return ip1 == ip2;
}

int is_broadcast(std::vector<uint8_t> address) {
    int result = 1;
    for(int i = 0; i < 6; i++) {
        if (address[i] != 0xFF) {
            return 0;
        }
    }
    return result;
}

int is_broadcast(uint32_t address) {
    return address == 0xFFFFFFFF;
}

int in_subnet(uint32_t ip_1,uint32_t ip_2, uint32_t test_netmask) {
    // roll through each of the bits of the netmask
    for (int i = 0; i < 32; i++){
        // check to see if this bit is 1 in the netmask, meaning
        // that an ip in the same subnet should be equal to the subnet
        // at this bit
        if ((test_netmask >> i) & 0x01){
            if (!(((ip_1 >> i) &  0x01) == ((ip_2 >> i) & 0x01))) {
                // if this is not true, then the address isn't in the same subnet
                return 0;
            }
        }
    }

    return 1;
}   

std::string mac_to_string(std::vector<uint8_t> mac_addr){

    std::ostringstream convert;
    for (int i = 0; i < 6; i++) {
        if (mac_addr[i] < 16) {
            convert << "0";
        }
        convert << std::uppercase << std::hex << (int)mac_addr[i];
        if (i < 5) {
            convert << ":";
        }
    }

    return convert.str();

}

std::string ip_to_string(uint32_t ip_addr){
    std::ostringstream convert;
    for (int i = 3; i >= 0; i--) {
        convert << (int)((ip_addr >> (i*8)) & 0x000000FF);
        if (i > 0) {
            convert << ".";
        }
    }

    return convert.str();
}


// int main() {

//     uint32_t ip_1 = create_ip(192,168,0,25);
//     uint32_t ip_2 = create_ip(192,168,0,105);

//     uint32_t netmask = create_ip(255,255,255,192);

//     std::cout << in_subnet(ip_1, ip_2, netmask) << std::endl;


//     return 0;
// }