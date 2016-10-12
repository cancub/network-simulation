#include <stdlib.h>
#include <iostream>
#include <vector>
#include "addressing.h"
#include <time.h>
#include <cstdint>
#include <sstream>

using namespace std;

std::vector<uint8_t> random_mac() {
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

std::vector<uint8_t> uniform_mac(uint8_t mac_byte) {
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 
    for (int i = 0; i < 6; i++) {
        result.push_back(mac_byte);
    }
    return result;
}

std::vector<uint8_t> broadcast_mac() {
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 
    for (int i = 0; i < 6; i++) {
        result.push_back(0xFF);
    }
    return result;
}

std::vector<uint8_t> random_ip() {
    // again, as one might expect, this takes no argument
    // and returns a random ip

    std::vector<uint8_t> result; // this will only be four bytes
    result.reserve(4);

    for(int i = 0; i < 4; i++) {
        result.push_back((uint8_t) (rand() % 256));
    }

    return result;
}

std::vector<uint8_t> uniform_ip(uint8_t ip_byte) {
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(4); 
    for (int i = 0; i < 4; i++) {
        result.push_back(ip_byte);
    }
    return result;
}

std::vector<uint8_t> broadcast_ip() {
    std::vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(4); 
    for (int i = 0; i < 4; i++) {
        result.push_back(0xFF);
    }
    return result;
}

int compare_macs(std::vector<uint8_t> mac1, std::vector<uint8_t> mac2) {
    int result = 0;
    for (int i = 0; i < 6; i++){
        if (mac1[i] != mac2[i]){
            return 1;
        }
    }
    return result;
}

int compare_ips(std::vector<uint8_t> ip1, std::vector<uint8_t> ip2) {
    int result = 0;
    for (int i = 0; i < 6; i++){
        if (ip1[i] != ip2[i]){
            return 1;
        }
    }
    return result;
}

int is_broadcast(std::vector<uint8_t> address) {
    int result = 0;
    for(std::vector<uint8_t>::iterator it = address.begin(); it != address.end(); ++it) {
        if (*it != 0xFF) {
            return 1;
        }
    }
    return result;
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

std::string ip_to_string(std::vector<uint8_t> ip_addr){
    std::ostringstream convert;
    for (int i = 0; i < 4; i++) {
        convert << (int)ip_addr[i];
        if (i < 3) {
            convert << ".";
        }
    }

    return convert.str();
}


// int main() {

//     srand(time(NULL));
//     uint8_t byte = 45;
//     std::vector<uint8_t> my_mac1 = uniform_mac(byte);
//     std::vector<uint8_t> my_mac2 = broadcast_mac();

//     cout << mac_to_string(my_mac1) << " " << is_broadcast(my_mac1) << endl;
//     cout << mac_to_string(my_mac2) << " " << is_broadcast(my_mac2) << endl;

//     return 0;
// }