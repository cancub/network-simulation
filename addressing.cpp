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

std::string mac_to_string(std::vector<uint8_t> mac_addr){

    std::ostringstream convert;
    for (int i = 0; i < 6; i++) {
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


int main() {

    srand(time(NULL));
    std::vector<uint8_t> my_mac1;
    std::vector<uint8_t> my_mac2;
    std::vector<uint8_t> my_mac3;
    uint8_t byte = 45;
    my_mac1 = uniform_mac(byte);
    my_mac2 = uniform_mac(byte);
    my_mac3 = random_mac();

    cout << compare_macs(my_mac1,my_mac2) << " " << compare_macs(my_mac1,my_mac3) << endl;

    return 0;
}