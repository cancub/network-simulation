#include <stdlib.h>
#include <iostream>
#include <vector>
#include <thread>
#include <cmath>
#include <algorithm>
#include <unistd.h>
#include <string>
#include <chrono>
#include "host.h"
#include "frame_generators.h"
#include "pdu.h"
#include <iomanip>
#include <cstdint>
#include "data_links.h"
#include "addressing.h"
#include "l3_protocols.h"

using namespace std;

#define POISSON 10 // the default lambda that we'll work with for now
#define MTU     1500    // the maximum transmission unit describes how long a frame can be
#define FRAME_SIZE_MIN 60 

#define DEFAULT_TIMEOUT 1 

// #define DEBUG
// #define TEST_SENDING
#define TEST_PINGS

#define NUMBER_OF_FRAMES 100 // the number of frames to be sent by the host in a test


Host::Host() {
    rx_frame_count = 0;
    frame_generator = new Poisson(POISSON);
    host_start_time = std::chrono::high_resolution_clock::now();
    name = "unnamed";
    frame_queue = new wqueue<MPDU*>;
#ifdef DEBUG
    host_print("Online");
#endif
}

Host::Host(uint32_t ip_addr, std::vector<uint8_t> mac_addr, std::string hostname) {
    ip = ip_addr;
    mac = mac_addr;
    rx_frame_count = 0;
    name = hostname;
    frame_generator = new Poisson(POISSON);
    host_start_time = std::chrono::high_resolution_clock::now();
    frame_queue = new wqueue<MPDU*>;
#ifdef DEBUG
    // host_print("Online with MAC " + mac);
#endif
}


Host::~Host() {
    delete frame_generator;
    delete frame_queue;
}

void Host::run(uint32_t dest_ip) {
    // make the threads for receiving and demuxing/sending
    std::thread receiver_thread(&Host::receiver,this);
    std::thread mux_demux_thread(&Host::muxer_demuxer,this);

    // send some arps
    if (!is_broadcast(dest_ip)) {
        arping(dest_ip,3,DEFAULT_TIMEOUT);
    }

    // let them run
    receiver_thread.join();
    mux_demux_thread.join();
}

uint32_t Host::get_ip() {return ip;}

std::vector<uint8_t> Host::get_mac() {return mac;}

int Host::get_frame_count() {return rx_frame_count;}

void Host::set_ip(uint32_t ip_addr) {
    ip = ip_addr;
}

void Host::set_mac(std::vector<uint8_t> mac_addr) {
    mac = mac_addr;
}

void Host::set_port(Ethernet* tx_if, Ethernet* rx_if) {
    // a port can be defined by its outgoing and incoming wires for the ethernet link
    tx_interface = tx_if;
    rx_interface = rx_if;
}

void Host::receiver() {
    // this is to be run in a thread that will wait on a condition variable to check the rx
    // interface. 
    while (1) {
        frame_queue->add(rx_interface->receive());
    }

}

void Host::muxer_demuxer() {
    // checks the main frame queue and moves MPDUs accordingly.
    // MPDUs whose source MAC address matches the host address 
    // transfered out of the host and all others are processed

    MPDU* frame;
    std::vector<uint8_t> dst_mac;

    while(1) {
        // get the next frame
        frame = frame_queue->remove();
#ifdef DEBUG
        host_print("got frame");
#endif

        // find out if this frame is from the host
        if (compare_macs(frame->get_src_mac(), mac)) {
            // this frame came from the host and therefore
            // must be sent out the tx_interface
            send_MPDU(frame);
        } else {
            // this frame is inbound.
            // first thing's first: check to see if the frame should be inspected
            // (if the frame is broadcast or specifically destined for the host)

            dst_mac = frame->get_dst_mac();

            if (compare_macs(dst_mac, mac) || is_broadcast(dst_mac)) {
                process_frame(frame, dst_mac);
            }
        }
    } 
}

void Host::send_MPDU(MPDU* frame) {

#ifdef DEBUG
    host_print("sending " +std::to_string(frame->get_size()) + " bytes to " + 
        mac_to_string(frame->get_dst_mac()));
#endif
    // and that frame is sent out on the link
    tx_interface->transmit(frame);
}

void Host::process_frame(MPDU* frame, std::vector<uint8_t> dst_mac) {
    // the contents of received frames will be processed in this function

    std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - host_start_time;

    increment_frame_count();

    // this frame was destined for this host, so print some details about the frame

    std::string frame_descriptor;

    if (is_broadcast(dst_mac)) {
        frame_descriptor = "BROADCAST";
    } else {
        frame_descriptor = "UNICAST from " + mac_to_string(dst_mac);
    }

    std::string statement = std::to_string(get_frame_count()) + " " + std::to_string(diff.count()) 
                            + " " + mac_to_string(frame->get_src_mac())
                             + " " + std::to_string(frame->get_size()) + " " + frame_descriptor;
    host_print(statement);

       
    // here is where something would be done with the frame (e.g. go to running ping process)
    // switch(frame->get_SDU_type()) {
    //     case 0x0806:
    //         receive_arp(frame->get_SDU());
    //         break;
    // }
    if (frame->get_SDU_type() == 0x0806) {
        receive_arp(frame->get_SDU());
    }

    delete frame;
}

void Host::increment_frame_count() {
    rx_frame_count += 1;
}

void Host::ping(uint32_t dest_ip, int count) {
/*
    - ping (icmp request/reply) requires different things for different things when crafting packets:
        - for packets whose destination address is within the same subnet, the mac address is needed 
            and thus an ARP message is needed prior to sending if the address is not in the local
            ARP cache (which must be flushed every so often)
        - for other packets (such as those desitned for the internet), the ip address is all that is needed
            and then the first hop router will be the destination address
        - for the sake of this network simulator, we'll only consider the first case (local pings)
        - ping, arp, etc tools should just be part of the host as a tool that can be used
        - replies will be fed into these members
        - so ping goes thusly:
            - host creates a frame object, starts a thread with ping member, giving
                it frame, ip and number of pings to make
            - ping generates icmp request frame 
            - ping copies this into the SDU of NetworkPDU, sets the size, destination and source ip
            - ping copies this into the SDU of the MPDU, sets the size
        - ping requires MAC, so if there exists an entry in the ARP cache for this IP, use the MAC address,
            otherwise send an ARP to get the MAC and wait on the response before sending ping 
        - maybe timeout

*/

    // create an ICMP echo request frame
    ICMP request_frame;
    IP ip_to_send;
    MPDU* ping_mpdu = new MPDU();

    request_frame.code = ICMP_REQUEST;
    request_frame.sequence_number = 0;
    // fill_ICMP_payload(request_frame);


    std::vector<uint8_t> dest_mac;

    // check to see how this ping should be handled
    if (in_subnet(ip,dest_ip,netmask)) {
        // in the same subnet, so get the mac
        dest_mac = cache.get_mac(dest_ip);
        // test to see if the ip has actually been entered into the cache
        // and can thus be pinged without arping
        if (is_broadcast(dest_mac)) {
            // there was no entry, so send an arp to get the mac
            send_arp(dest_ip, DEFAULT_TIMEOUT);
            // wait until the mac address has been found and added to the queue
            while (is_broadcast(cache.get_mac(dest_ip))) {
                usleep(1000);
            }
            dest_mac = cache.get_mac(dest_ip);
        }
        // encapsulate the ICMP as IP frame
    } else {
        // it isn't in the same subnet, so just send to the router
        // and let the rest of the network/internet take care of it
        dest_mac = router_mac;
    }

    //encap 
    ip_to_send.encap_SDU(request_frame);
    ip_to_send.set_dest_ip(dest_ip);
    ip_to_send.set_src_ip(ip);

    // encapsulate it in an MPDU
    ping_mpdu->encap_SDU(ip_to_send);

    // send it as many times as requested
    for (int i = 0; i < count; i++) {
        send_MPDU(ping_mpdu);
        // sleep for a second (can change this later on)
        usleep(1000000);
    }


}

void Host::arping(uint32_t requested_ip, int count, int timeout) {
    for (int i = 0; i < count; i++){
        send_arp(requested_ip, timeout);
#ifdef DEBUG
        host_print("ARP sent to " + ip_to_string(requested_ip));
#endif
        usleep(1000000);
    }
}

void Host::send_arp(uint32_t requested_ip, int timeout) {
    // create ARP request
    ARP arp;
    arp.opcode = 0x0001;
    arp.sender_mac = mac;
    arp.sender_ip = ip;
    arp.target_mac = create_uniform_mac(0);
    arp.target_ip = requested_ip;

    send_arp(arp,create_broadcast_mac(), timeout);
}

void Host::send_arp(ARP tx_arp, std::vector<uint8_t> dst_mac, int timeout) {

    // pack it in an IP frame
    MPDU* arp_mpdu = new MPDU;
    arp_mpdu->set_src_mac(mac);
    arp_mpdu->set_dst_mac(dst_mac);
    arp_mpdu->encap_SDU(tx_arp);

    // send it on its merry way
    frame_queue->add(arp_mpdu);
}

void Host::receive_arp(std::vector<uint8_t> arp_u8) {

    
    ARP arp_rx = generate_ARP(arp_u8);

    // first, we should check that the target is indeed this host

    // cout << ip_to_string(arp_rx.target_ip) << " " << ip_to_string(ip) << endl;

    if (compare_ips(arp_rx.target_ip, ip)) {


        // check to see if the sending address pair exists in the cache
        std::vector<uint8_t> search_mac = cache.get_mac(arp_rx.sender_ip);

        // add if it does not exist, add it
        if (is_broadcast(search_mac)) {
            cache.add_entry(arp_rx.sender_ip, search_mac);
        }
     

        // check the opcode to see if this is a reply or a request
        // a reply means that we should do nothing but add the result to the
        // cache whereas a request means that we should both add the info on
        // the sender to the cache as well as send a reply
        if (arp_rx.opcode == 0x0001) {
            // request

            // modify opcode
            arp_rx.opcode = 0x0002;
            // flip addresses
            arp_rx.target_mac = arp_rx.sender_mac;
            arp_rx.target_ip = arp_rx.sender_ip;
            // set this hosts addresses
            arp_rx.sender_mac = mac;
            arp_rx.sender_ip = ip;

            // send off the reply
            send_arp(arp_rx, arp_rx.target_mac, DEFAULT_TIMEOUT);
        } else {
            host_print("Unicast reply from " + ip_to_string(arp_rx.sender_ip) + " [" 
                + mac_to_string(arp_rx.sender_mac) + "]");
        }
    }
}

void Host::host_print(std::string statement) {
    std::cout << setw(15) << name << ": " << statement << endl;
}


// int main() {
//     // create 

//     // give it an address to ping

//     // make sure the frame is sent and the timeout message occurs
// }