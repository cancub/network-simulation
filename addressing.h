#ifndef ADDRESSING_H
#define ADDRESSING_H

// #include <stdlib.h>
// #include <iostream>
// #include <vector>
#include <string>
#include <cstdint>

std::vector<uint8_t> random_mac();
std::vector<uint8_t> uniform_mac(uint8_t);
std::vector<uint8_t> broadcast_mac();
std::vector<uint8_t> random_ip();
std::vector<uint8_t> uniform_ip(uint8_t);
std::vector<uint8_t> broadcast_ip();

int compare_macs(std::vector<uint8_t>, std::vector<uint8_t>);
int compare_ips(std::vector<uint8_t>, std::vector<uint8_t>);
int is_broadcast(std::vector<uint8_t>);

std::string mac_to_string(std::vector<uint8_t>);
std::string ip_to_string(std::vector<uint8_t>);

#endif