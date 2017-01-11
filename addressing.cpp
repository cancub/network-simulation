#include <stdlib.h>
#include <iostream>
#include <vector>
#include "addressing.h"
#include <time.h>
#include <cstdint>
#include <sstream>

using namespace std;

vector<uint8_t> create_mac(uint8_t b1,uint8_t b2,uint8_t b3,uint8_t b4,uint8_t b5,uint8_t b6) {
    vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 

    result.push_back(b1);
    result.push_back(b2);
    result.push_back(b3);
    result.push_back(b4);
    result.push_back(b5);
    result.push_back(b6);

    return result;
}

vector<uint8_t> create_random_mac() {
    // as one might expect, this is a function
    // that takes no argument and generates
    // a random MAC address 
    
    int character_number;
    vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 

    for(int i = 0; i < 6; i++) {
        result.push_back((uint8_t)(rand() % 256));
    }

    return result;
}  

vector<uint8_t> create_uniform_mac(uint8_t mac_byte) {
    vector<uint8_t> result; // we'll add 6 bytes to this mac
    result.reserve(6); 
    for (int i = 0; i < 6; i++) {
        result.push_back(mac_byte);
    }
    return result;
}

vector<uint8_t> create_broadcast_mac() {
    vector<uint8_t> result; // we'll add 6 bytes to this mac
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

int compare_macs(vector<uint8_t> mac1, vector<uint8_t> mac2) {
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

int is_broadcast(vector<uint8_t> address) {
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

string mac_to_string(vector<uint8_t> mac_addr){

    ostringstream convert;
    for (int i = 0; i < 6; i++) {
        if (mac_addr[i] < 16) {
            convert << "0";
        }
        convert << uppercase << hex << (int)mac_addr[i];
        if (i < 5) {
            convert << ":";
        }
    }

    return convert.str();

}

string mac_to_string(vector<uint8_t>* mac_vector, int first_index){

    ostringstream convert;
    uint8_t current_byte;
    for (int i = first_index; i < first_index+ 6; i++) {
        current_byte = mac_vector->at(i);
        if (current_byte < 16) {
            convert << "0";
        }
        convert << uppercase << hex << (int)current_byte;
        if (i < first_index+ 5) {
            convert << ":";
        }
    }

    return convert.str();

}

string ip_to_string(uint32_t ip_addr){
    ostringstream convert;
    for (int i = 3; i >= 0; i--) {
        convert << (int)((ip_addr >> (i*8)) & 0x000000FF);
        if (i > 0) {
            convert << ".";
        }
    }

    return convert.str();
}

string ip_to_string(vector<uint8_t> * ip_vector, int first_index) {
    ostringstream convert;
    for (int i = first_index; i < first_index + 4; i++) {
        convert << (int)(ip_vector->at(i));
        if (i < first_index+3) {
            convert << ".";
        }
    }

    return convert.str();
}


// int main() {

//     vector<uint8_t> test;
//     test.push_back(0x11);
//     test.push_back(0x22);
//     test.push_back(0x33);
//     test.push_back(0x44);
//     test.push_back(0x44);
//     test.push_back(0x55);


//     cout << mac_to_string(&test,0) << endl;


//     return 0;
// }