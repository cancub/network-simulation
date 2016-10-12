#ifndef FRAMES_H
#define FRAMES_H

#include <stdlib.h>
#include <string>
#include <cstdint>
#include <vector>

/*
This is the class that will be our interface, since what is an interface if not
just a pointer to a frame on a link?
*/

using namespace std;


class Frame {
    public:
        Frame(); // default constructor
        Frame(std::vector<uint8_t>); // constructor with the source MAC address
        Frame(std::vector<uint8_t>, size_t); // constructor with the source MAC address and the length of the frame
        Frame(std::vector<uint8_t>, std::vector<uint8_t>, size_t); // constructor with the source MAC address, 
                                                // the destination address and the length of the frame
        ~Frame();
        std::vector<uint8_t> get_src_mac();
        std::vector<uint8_t> get_dst_mac();
        std::vector<uint8_t> get_src_ip();
        std::vector<uint8_t> get_dst_ip();
        size_t get_frame_size();
        void set_src_mac(std::vector<uint8_t>);
        void set_dst_mac(std::vector<uint8_t>);
        void set_src_ip(std::vector<uint8_t>);
        void set_dst_ip(std::vector<uint8_t>);
        void set_frame_size(size_t);
        void erase();   // reset the frame to be blank
        Frame* copy();
    private:
        std::vector<uint8_t> source_mac;
        std::vector<uint8_t> destination_mac;
        std::vector<uint8_t> source_ip;
        std::vector<uint8_t> destination_ip;
        size_t frame_size;
};

#endif