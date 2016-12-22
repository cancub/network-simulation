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

#define DEBUG
// #define TEST_SENDING
// #define FRAME_TIMES

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
    host_print("Online with MAC " + mac_to_string(mac));
#endif
}


Host::~Host() {
    delete frame_generator;
    delete frame_queue;
}

void Host::run(uint32_t destination_ip, int delay) {

    // make the threads for receiving and demuxing/sending
    std::thread receiver_thread(&Host::receiver,this);
    std::thread mux_demux_thread(&Host::demuxer,this);

    usleep(delay);

    // send some arps
    if (!is_broadcast(destination_ip)) {
        // arping(destination_ip,3,DEFAULT_TIMEOUT);
#ifdef DEBUG
        host_print("performing action");
#endif
        ping(destination_ip,10);
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

void Host::demuxer() {
    // checks the main frame queue and moves MPDUs accordingly.
    // MPDUs whose source MAC address matches the host address 
    // transfered out of the host and all others are processed

    MPDU* frame;
    std::vector<uint8_t> destination_mac;

    while(1) {
        // get the next frame
        frame = frame_queue->remove();
        // this frame is inbound.
        // first thing's first: check to see if the frame should be inspected
        // (if the frame is broadcast or specifically destined for the host)
#ifdef DEBUG
        host_print("got rx frame");
#endif
        destination_mac = frame->get_destination_mac();

        if (compare_macs(destination_mac, mac) || is_broadcast(destination_mac)) {
            process_frame(frame, destination_mac);
        }
        
    } 
}

void Host::send_MPDU(MPDU* frame) {

#ifdef DEBUG
    host_print("sending " +std::to_string(frame->get_size()) + " bytes to " + 
        mac_to_string(frame->get_destination_mac()));
#endif
    // and that frame is sent out on the link
    tx_interface->transmit(frame);
}

void Host::process_frame(MPDU* frame, std::vector<uint8_t> destination_mac) {
    // the contents of received frames will be processed in this function

    std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - host_start_time;

    increment_frame_count();

    // this frame was destined for this host, so print some details about the frame

#ifdef FRAME_TIMES
    std::string frame_descriptor;

    if (is_broadcast(destination_mac)) {
        frame_descriptor = "BROADCAST";
    } else {
        frame_descriptor = "UNICAST from " + mac_to_string(destination_mac);
    }

    std::string statement = std::to_string(get_frame_count()) + " " + std::to_string(diff.count()) 
                            + " " + mac_to_string(frame->get_source_mac())
                             + " " + std::to_string(frame->get_size()) + " " + frame_descriptor;
    host_print(statement);
#endif

       
    // here is where something would be done with the frame (e.g. go to running ping process)
    switch(frame->get_SDU_type()) {
        case 0x0806:
            receive_arp(frame->get_SDU());
            delete frame;
            break;

        case 0x0800: // IP packet  
            // use the protocol of the frame to figure out how to proceed with the contents
            IP packet(frame->get_SDU());
            int socket_found = 0;

            switch(packet.get_protocol()) {
                case IP_PROTOCOL_ICMP:
                    // this is a ping, so send the raw frame to the ICMP socket if it's open
                    for (std::vector<Socket*>::iterator it = open_ports.begin(); 
                        it != open_ports.end();++it) {

                        if ((*it)->get_protocol() == IP_PROTOCOL_ICMP) {
                            socket_found = 1;
                            // give this frame to the running ping process' socket
                            (*it)->add_frame(frame);
                        }
                    }

                    if (!socket_found) {
                        // there's no open socket for a running ping process, so just send it
                        // the generic ICMP processor and delete it
                        process_ICMP_message(frame);
                    }                                        
                    break;
            }
            break;
    }
    // if (frame->get_SDU_type() == 0x0806) {
    //     receive_arp(frame->get_SDU());
    // }
}

void Host::increment_frame_count() {
    rx_frame_count += 1;
}

void Host::fill_ping_payload(ICMP* ping_to_send) {

    ping_to_send->payload.clear();

    ping_to_send->payload.reserve(48);
    ping_to_send->payload.push_back(0xb5);
    ping_to_send->payload.push_back(0x32);
    ping_to_send->payload.push_back(0x05);

    for (int i = 0; i < 5; i++) {
        ping_to_send->payload.push_back(0x00);
    }

    for (int i = 0; i < 40; i++){
        ping_to_send->payload.push_back(16+i);
    }
}

void Host::ping(uint32_t destination_ip, int count) {

    std::vector<uint8_t> destination_mac;

    // check to see how this ping should be handled
    if (in_subnet(ip,destination_ip,netmask)) {
        // this means that the ip is in the same subnet, so get the mac
        destination_mac = cache.get_mac(destination_ip);

        // test to see if the ip has actually been entered into the cache
        // and can thus be pinged without arping
        if (is_broadcast(destination_mac)) {
#ifdef DEBUG
            host_print("[PING] No MAC in ARP cache for IP " + ip_to_string(destination_ip) + ". ARPING");
#endif
            // there was no entry, so send an arp to get the mac
            send_request_arp(destination_ip, DEFAULT_TIMEOUT);

            // wait until the mac address has been found and added to the queue
            while (is_broadcast(cache.get_mac(destination_ip))) {
                usleep(100);
            }

            // the mac has been obtained, so get it
            destination_mac = cache.get_mac(destination_ip);
        }

#ifdef DEBUG
        host_print("[PING] Got MAC " + mac_to_string(destination_mac));
#endif

    } else {
        // it isn't in the same subnet, so just send to the router
        // and let the rest of the network/internet take care of it
        destination_mac = router_mac;
    }


    // create an ICMP echo request frame
    ICMP ping_to_send;
    ping_to_send.type = ICMP_ECHO_REQUEST;
    ping_to_send.identifier = rand() % 65536;   // this ping will always have the same ID
    ping_to_send.sequence_number = 0;

    // set up the IP packet, the ICMP will have to be re-encapped at each iteration due to a change
    // in the sequence number and (hopfeully later) the payload and checksum
    IP ip_to_send;
    ip_to_send.set_destination_ip(destination_ip);
    ip_to_send.set_source_ip(ip);

    // create a socket that will be used to send 
    Socket* icmp_socket = create_socket(0, IP_PROTOCOL_ICMP);

    MPDU* rx_frame;

    ICMP    rx_ping;
    IP      rx_ip;

    std::vector<float> test_times;
    test_times.reserve(count);
    int transmission_attempts = 0;
    int receptions = 0;

    int check_count = count;

    // send it as many times as requested
    while (true) {
        ping_to_send.sequence_number++;

        fill_ping_payload(&ping_to_send);

        ip_to_send.encap_SDU(ping_to_send);

#ifdef DEBUG
        host_print("[PING] Sending ping " + to_string(ping_to_send.sequence_number)+ "/" + to_string(count));
#endif
        // encapsulate IP in an MPDU
        MPDU* ping_mpdu = new MPDU();
        ping_mpdu->encap_SDU(ip_to_send);
        ping_mpdu->set_source_mac(mac);
        ping_mpdu->set_destination_mac(destination_mac);
        std::chrono::time_point<std::chrono::high_resolution_clock> ping_send_time = 
            std::chrono::high_resolution_clock::now();

        send_MPDU(ping_mpdu);  
        transmission_attempts++;
        check_count--;


        do {
            // wait on the reply in the socket
            rx_frame = icmp_socket->get_frame();

            rx_ip = generate_IP(rx_frame->get_SDU());

            rx_ping = generate_ICMP(rx_ip.get_SDU());

            // figure out the type of the ICMP frame
            switch(rx_ping.type) {
                case ICMP_ECHO_REQUEST:
                    // send this raw frame to the function that processes requests
                    process_ICMP_message(rx_frame);
                break;

                case ICMP_ECHO_REPLY:
                    // in the future we'll have to somehow differentiate between
                    // multiple ping processess so we should check the id, but for now
                    // we should only be getting pings replies with the same id
                    if (rx_ping.identifier == ping_to_send.identifier) {
#ifdef DEBUG
                        host_print("[PING] Received ping reply for this process");
#endif
                        receptions++;
                    } else {
#ifdef DEBUG
                        host_print("[PING] Received ping reply for a different process " + 
                            to_string(rx_ping.identifier) + " " + to_string(ping_to_send.identifier));
#endif
                    }
                break;
            }

        } while (rx_ping.type != ICMP_ECHO_REPLY/*|| timeout*/);

        // print the results for this iteration, be it a clean reception or a timeout
        std::chrono::duration<double, std::milli> diff = std::chrono::high_resolution_clock::now() 
                                                                - ping_send_time;

        test_times.push_back((float)(diff.count()));

        host_print("[PING] " + to_string(rx_ip.get_total_length() - rx_ip.get_header_length()) + 
            " bytes from " + mac_to_string(rx_frame->get_source_mac()) + ": icmp_seq=" + 
            to_string(rx_ping.sequence_number) + " time=" + to_string(diff.count()) + " ms");

        delete rx_frame;

        if (check_count == 0) {
            break;
        }

        // sleep for a second (can change this later on)
        usleep(1000000);
    }

    delete_socket(icmp_socket->get_port());

    host_print("--- " + ip_to_string(destination_ip) + " ping statistics ---");
    print_ping_statistics(transmission_attempts, receptions, test_times);

}

void Host::print_ping_statistics(int transmissions, int receptions, std::vector<float> ping_times) {
    // print all the details surrounding the pings

    float total_time = 0;
    float min_time = ping_times[0];
    float max_time = ping_times[0];
    float mdev = 0;
    float avg_time;

    for (int i = 0; i < ping_times.size(); i++) {
        total_time += ping_times[i];

        if (ping_times[i] < min_time) {
            min_time = ping_times[i];
        } 

        if (ping_times[i] > max_time) {
            max_time = ping_times[i];
        } 
    }

    avg_time = total_time / ping_times.size();

    for (int i = 0; i < ping_times.size(); i++) {
        mdev += pow(ping_times[i] - avg_time, 2.0);
    }

    mdev /= ping_times.size();

    mdev = sqrt(mdev);

    host_print(to_string(transmissions) + " packets transmitted, " + to_string(receptions) + 
        " received, " + to_string((int)(1.0 - ((float)receptions)/((float)transmissions)) * 100) + 
        "% packet loss, time " + to_string(total_time) + " ms" );

    host_print("rtt min/avg/max/mdev = " + to_string(min_time) + "/" + 
        to_string(avg_time) + "/" + to_string(max_time) + "/" + to_string(mdev));
}

void Host::process_ICMP_message(MPDU* rx_frame) {

    IP rx_ip = generate_IP(rx_frame->get_SDU());


    ICMP inbound_ping = generate_ICMP(rx_ip.get_SDU());

    // check to see if this is a reply or a request as these will be handled differently

    switch(inbound_ping.type) {
        case ICMP_ECHO_REPLY:
            // since the only reason we got to ths point was because there wasn't a running
            // ping process, we can safely assume that this ping reply was received in error
#ifdef DEBUG
            host_print("[PING] received ICMP reply in error");
#endif   

            break;

        case ICMP_ECHO_REQUEST:
            // a request does not need to be checked, instead a reply is generated immediately and sent

            // don't waste time creating a new ping (ID is the same, payload is the same),
            // just change the type
            inbound_ping.type = ICMP_ECHO_REPLY;

            // encap in IP
            IP ping_ip;
            ping_ip.set_source_ip(ip);
            ping_ip.set_destination_ip(rx_ip.get_source_ip());
            ping_ip.encap_SDU(inbound_ping);

            // incap IP in MPDU
            MPDU* ping_mpdu = new MPDU();
            ping_mpdu->encap_SDU(ping_ip);
            ping_mpdu->set_source_mac(mac);
            ping_mpdu->set_destination_mac(rx_frame->get_source_mac());

#ifdef DEBUG
            host_print("[PING] received ICMP request, sending reply");
#endif
            // send it on its way
            send_MPDU(ping_mpdu);
            break;
    }

    delete rx_frame;
}

Socket* Host::create_socket(uint16_t requested_port_number = 1, uint8_t protocol_number = 0) {
    uint16_t test_port_number = requested_port_number;
    uint8_t port_found = 0;
    // scan through the open ports to get the next available port number and assign it
    if (open_ports.size() > 0) { 
        do
        {   
            for (std::vector<Socket*>::iterator it = open_ports.begin(); it != open_ports.end(); ++it) {
                if ((*it)->get_port() == test_port_number) {
                    // the port number has already been taken, so see if the next value works
                    port_found = 0;
                    test_port_number++; 
                    break;
                }
            }

        } while (port_found == 0);       
    }

    // test_port_number now contains the first available port number, so use that in creating the socket
    Socket* new_socket = new Socket(test_port_number, protocol_number);

    open_ports.push_back(new_socket);

    return new_socket;
}

void Host::delete_socket(uint16_t port_num) {
    for (std::vector<Socket*>::iterator it = open_ports.begin(); it != open_ports.end();) {
        if ((*it)->get_port() == port_num) {
            delete *it;
            it = open_ports.erase(it);
        } else {
            ++it;
        }
    }
}

void Host::arping(uint32_t requested_ip, int count, int timeout) {
    for (int i = 0; i < count; i++){
        send_request_arp(requested_ip, timeout);
        usleep(1000000);
    }
}

void Host::send_request_arp(uint32_t requested_ip, int timeout) {
    // create ARP request
    ARP arp;
    arp.opcode = ARP_REQUEST;
    arp.sender_mac = mac;
    arp.sender_ip = ip;
    arp.target_mac = create_uniform_mac(0);
    arp.target_ip = requested_ip;


// #ifdef DEBUG
    host_print("[ARP] Sending request to " + ip_to_string(requested_ip));
// #endif

    send_arp(arp,create_broadcast_mac());
}

void Host::send_arp(ARP tx_arp, std::vector<uint8_t> destination_mac) {

    // pack it in an IP frame
    MPDU* arp_mpdu = new MPDU;
    arp_mpdu->set_source_mac(mac);
    arp_mpdu->set_destination_mac(destination_mac);
    arp_mpdu->encap_SDU(tx_arp);

    // send it on its merry way
    send_MPDU(arp_mpdu);
}

void Host::receive_arp(std::vector<uint8_t> arp_u8) {

    
    ARP arp_rx = generate_ARP(arp_u8);

    // first, we should check that the target is indeed this host
    if (compare_ips(arp_rx.target_ip, ip)) {

        // check to see if the sending address pair exists in the cache
        // add if it does not exist, add it
        if (is_broadcast(cache.get_mac(arp_rx.sender_ip))) {
            cache.add_entry(arp_rx.sender_ip, arp_rx.sender_mac);
        }
     

        // check the opcode to see if this is a reply or a request
        // a reply means that we should do nothing but add the result to the
        // cache whereas a request means that we should both add the info on
        // the sender to the cache as well as send a reply
        if (arp_rx.opcode == ARP_REQUEST) {
            // request

            // modify opcode
            arp_rx.opcode = ARP_REPLY;
            // flip addresses
            arp_rx.target_mac = arp_rx.sender_mac;
            arp_rx.target_ip = arp_rx.sender_ip;
            // set this hosts addresses
            arp_rx.sender_mac = mac;
            arp_rx.sender_ip = ip;

#ifdef DEBUG
            host_print("[ARP] Sending reply to " + ip_to_string(arp_rx.target_ip));
#endif
            // send off the reply
            send_arp(arp_rx, arp_rx.target_mac);
        } else {
            host_print("[ARP] Unicast reply from " + ip_to_string(arp_rx.sender_ip) + " [" 
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