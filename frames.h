#ifndef FRAMES_H
#define FRAMES_H

#include <stdlib.h>
#include <string>

/*
This is the class that will be our interface, since what is an interface if not
just a pointer to a frame on a link?
*/

class Frame {
    public:
        Frame(); // default constructor
        Frame(std::string); // constructor with the source MAC address
        Frame(std::string, size_t); // constructor with the source MAC address and the length of the frame
        Frame(std::string, std::string, size_t); // constructor with the source MAC address, 
                                                // the destination address and the length of the frame
        ~Frame();
        std::string get_src_mac();
        std::string get_dst_mac();
        std::string get_src_ip();
        std::string get_dst_ip();
        size_t get_frame_size();
        void set_src_mac(std::string);
        void set_dst_mac(std::string);
        void set_src_ip(std::string);
        void set_dst_ip(std::string);
        void set_frame_size(size_t);
        void erase();   // reset the frame to be blank
        Frame* copy();
    private:
        std::string source_mac;
        std::string destination_mac;
        std::string source_ip;
        std::string destination_ip;
        size_t frame_size;
};

#endif