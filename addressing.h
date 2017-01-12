#ifndef ADDRESSING_H
#define ADDRESSING_H

// #include <stdlib.h>
// #include <iostream>
// #include <vector>
#include <string>
#include <cstdint>
#include <vector>

using namespace std;

vector<uint8_t> create_mac(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
vector<uint8_t> create_random_mac();
vector<uint8_t> create_uniform_mac(uint8_t);
vector<uint8_t> create_broadcast_mac();

uint32_t create_ip(uint8_t,uint8_t, uint8_t, uint8_t);
uint32_t create_random_ip();
uint32_t create_broadcast_ip();

bool compare_macs(vector<uint8_t>, vector<uint8_t>);
bool compare_ips(uint32_t, uint32_t);
bool is_broadcast(vector<uint8_t>);
bool is_broadcast(uint32_t);

bool in_subnet(uint32_t,uint32_t, uint32_t);

string mac_to_string(vector<uint8_t>);
string mac_to_string(vector<uint8_t> *, int);
string ip_to_string(uint32_t);
string ip_to_string(vector<uint8_t> *, int);

#endif