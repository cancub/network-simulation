#include <string>
#include "frames.h"
#include <iostream>

/*
    Frames are the objects that will move around the network, being
    created by a source host and delated by the receiving host
*/

Frame::Frame() {
    source_mac = "";
    destination_mac = "";
    frame_size = 0;
}

Frame::Frame(std::string src) {
    source_mac = src;
    destination_mac = "";
    frame_size = 0;
}

Frame::Frame(std::string src, size_t length) {
    source_mac = src;
    destination_mac = "";
    frame_size = length;
}

Frame::Frame(std::string src, std::string dst, size_t length) {
    source_mac = src;
    destination_mac = dst;
    frame_size = length;
}

Frame::~Frame() {}

std::string Frame::get_src_mac() { return source_mac; }

std::string Frame::get_dst_mac() { return destination_mac; }

std::string Frame::get_src_ip() { return source_ip; }

std::string Frame::get_dst_ip() { return destination_ip; }
        
size_t Frame::get_frame_size() {return frame_size;}

void Frame::set_src_mac(std::string mac) {
    source_mac = mac;
}

void Frame::set_dst_mac(std::string mac) {
    destination_mac = mac;
}

void Frame::set_src_ip(std::string ip) {
    source_ip = ip;
}

void Frame::set_dst_ip(std::string ip) {
    destination_ip = ip;
}

void Frame::set_frame_size(size_t length) {
    frame_size = length;
}

void Frame::erase() {
    frame_size = 0;
    source_mac = "";
    source_ip = "";
    destination_mac = "";
    destination_ip = "";
}

Frame* Frame::copy() {
    // sometimes a frame needs to be sent out multiple interfaces, so it will need to be copied
    return new Frame(*this);
}