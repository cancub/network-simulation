#ifndef HOST_H
#define HOST_H

#include <string>
#include <mutex>
#include "frame_generators.h"
#include "frames.h"
#include <condition_variable> // std::condition_variable


/*
This is one of the big players in the network simulation. Without hosts,
there's no reason for a network. Hosts need to generate frames, access
links and read frames. That's pretty much it at the beggining. Later on
we'll add something to react to specific situations or events, like a ping
*/

class Host{
    public:
        Host(); // default constructor
        Host(std::string, std::string, std::string); // IP, MAC, and name (like "alice")
        // Host(std::string, std::string, std::string, std::mutex*, Frame*); // same as above but with a
                                                                            // mutex and interface
        ~Host();
        void run(std::string); // start sending frames to the specified MAC address
        std::string get_ip();
        std::string get_mac();
        int get_frame_count();
        void set_ip(std::string);
        void set_mac(std::string);
        void set_rx_interface(Frame* frame_interface, std::mutex* if_mutex, std::condition_variable* cv);
        void set_tx_interface(Frame* frame_interface, std::mutex* if_mutex, std::condition_variable* cv);
        void sender(std::string dst_mac);
        void receiver();
    private:
        void send_frame(int, std::string); // sending a frame to a specific MAC address
        // should split the above to unicast and broadcast (maybe multicast?)
        void process_frame(); // process an incoming frames
        void increment_frame_count();
        void mutex_sleep(); // sleep a random amount of microseconds so that there's less problems
        void host_print(std::string);
        Poisson* frame_generator; // the object that will create all the interarrival times
        Frame* rx_interface; // the location where the frames will be found/put on
        std::mutex* rx_interface_mutex; // mutex to lock before interacting with interface
        std::condition_variable* rx_condition_variable;
        Frame* tx_interface; // the location where the frames will be found/put on
        std::mutex* tx_interface_mutex; // mutex to lock before interacting with interface
        std::condition_variable* tx_condition_variable;
        std::string ip;
        std::string mac;
        std::chrono::time_point<std::chrono::high_resolution_clock> host_start_time; // keep track of time
        int rx_frame_count; // keep track of number of frames received
        std::string name; // the name of the host
};

#endif