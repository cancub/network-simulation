#ifndef ADDRESSING_H
#define ADDRESSING_H

// #include <stdlib.h>
// #include <iostream>
// #include <vector>
#include <string>
#include <cstdint>

std::vector<uint8_t> create_mac(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
std::vector<uint8_t> create_random_mac();
std::vector<uint8_t> create_uniform_mac(uint8_t);
std::vector<uint8_t> create_broadcast_mac();

uint32_t create_ip(uint8_t,uint8_t, uint8_t, uint8_t);
uint32_t create_random_ip();
uint32_t create_broadcast_ip();

int compare_macs(std::vector<uint8_t>, std::vector<uint8_t>);
int compare_ips(uint32_t, uint32_t);
int is_broadcast(std::vector<uint8_t>);
int is_broadcast(uint32_t);

std::string mac_to_string(std::vector<uint8_t>);
std::string ip_to_string(uint32_t);

#endif