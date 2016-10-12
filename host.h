#ifndef HOST_H
#define HOST_H

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include "frame_generators.h"
#include "frames.h"
#include <condition_variable> // std::condition_variable
#include "data_links.h"


/*
This is one of the big players in the network simulation. Without hosts,
there's no reason for a network. Hosts need to generate frames, access
links and read frames. That's pretty much it at the beggining. Later on
we'll add something to react to specific situations or events, like a ping
*/

class Host{
    public:
        Host(); // default constructor
        Host(std::vector<uint8_t>, std::vector<uint8_t>, std::string); // IP, MAC, and name (like "alice")
        // Host(std::string, std::string, std::string, std::mutex*, Frame*); // same as above but with a
                                                                            // mutex and interface
        ~Host();
        void run(std::vector<std::vector<uint8_t>>*); // start sending frames to the specified MAC address
        std::vector<uint8_t> get_ip();
        std::vector<uint8_t> get_mac();
        int get_frame_count();
        void set_ip(std::vector<uint8_t>);
        void set_mac(std::vector<uint8_t>);
        void set_port(Ethernet*, Ethernet*);
        void sender(std::vector<std::vector<uint8_t>>*);
        void receiver();
    private:
        void send_frame(int, std::vector<uint8_t>); // sending a frame to a specific MAC address
        // should split the above to unicast and broadcast (maybe multicast?)
        void process_frame(Frame*); // process an incoming frames
        void increment_frame_count();
        int run_DHCP_handshake();
        void host_print(std::string);
        Poisson* frame_generator; // the object that will create all the interarrival times
        Ethernet* rx_interface; // the location where the frames will be found/put on
        Ethernet* tx_interface; // the location where the frames will be found/put on
        std::vector<uint8_t> ip;
        std::vector<uint8_t> mac;
        std::chrono::time_point<std::chrono::high_resolution_clock> host_start_time; // keep track of time
        int rx_frame_count; // keep track of number of frames received
        std::string name; // the name of the host
};

#endif