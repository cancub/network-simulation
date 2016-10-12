#include <string>
#include "frames.h"
#include <iostream>
#include <cstdint>
#include <vector>

/*
    Frames are the objects that will move around the network, being
    created by a source host and delated by the receiving host
*/

using namespace std;

Frame::Frame() {
    source_mac.reserve(6);
    destination_mac.reserve(6);
    frame_size = 0;
}

Frame::Frame(std::vector<uint8_t> src) {
    source_mac = src;
    destination_mac.reserve(6);
    frame_size = 0;
}

Frame::Frame(std::vector<uint8_t> src, size_t length) {
    source_mac = src;
    destination_mac.reserve(6);
    frame_size = length;
}

Frame::Frame(std::vector<uint8_t> src, std::vector<uint8_t> dst, size_t length) {
    source_mac = src;
    destination_mac = dst;
    frame_size = length;
}

Frame::~Frame() {}

std::vector<uint8_t> Frame::get_src_mac() { return source_mac; }

std::vector<uint8_t> Frame::get_dst_mac() { return destination_mac; }

std::vector<uint8_t> Frame::get_src_ip() { return source_ip; }

std::vector<uint8_t> Frame::get_dst_ip() { return destination_ip; }
        
size_t Frame::get_frame_size() {return frame_size;}

void Frame::set_src_mac(std::vector<uint8_t> mac) {
    source_mac = mac;
}

void Frame::set_dst_mac(std::vector<uint8_t> mac) {
    destination_mac = mac;
}

void Frame::set_src_ip(std::vector<uint8_t> ip) {
    source_ip = ip;
}

void Frame::set_dst_ip(std::vector<uint8_t> ip) {
    destination_ip = ip;
}

void Frame::set_frame_size(size_t length) {
    frame_size = length;
}

void Frame::erase() {
    frame_size = 0;
    source_mac.clear();
    source_mac.reserve(6);
    source_ip.clear();
    source_ip.reserve(4);
    destination_mac.clear();
    destination_mac.reserve(6);
    destination_ip.clear();
    destination_ip.reserve(4);
}

Frame* Frame::copy() {
    // sometimes a frame needs to be sent out multiple interfaces, so it will need to be copied
    return new Frame(*this);
}