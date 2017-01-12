#include <stdlib.h>
#include <iostream> // printing
#include <vector>
#include <thread> // threading
#include <cmath>
#include <algorithm>
#include <unistd.h>
#include <cstdio> // HOST_HOST_EOF
#include <string>
#include <chrono> // fine grained timing
#include "host.h"
#include "frame_generators.h"
#include "pdu.h"
#include <iomanip>
#include <cstdint>
#include "data_links.h"
#include "addressing.h"
#include "l3_protocols.h"
#include "l4_protocols.h"
#include "md5.h"
#include <fstream>

using namespace std;

#define POISSON 10 // the default lambda for poisson random traffic
#define MTU     1500    // the maximum transmission unit describes how long a frame can be
#define FRAME_SIZE_MIN 60 
#define HOST_EOF 0xFF

#define DEFAULT_TIMEOUT 1 

// #define DEBUG
// #define L4_DEBUG
// #define ARP_DEBUG
// #define TEST_SENDING
// #define FRAME_TIMES

#define NUMBER_OF_FRAMES 100 // the number of frames to be sent by the host in a test


Host::Host() {
    rx_frame_count = 0;
    frame_generator = new Poisson(POISSON);
    host_start_time = chrono::high_resolution_clock::now();
    name = "unnamed";
    frame_queue = new wqueue<MPDU*>;
#ifdef DEBUG
    host_print("Online");
#endif
}

Host::Host(uint32_t ip_addr, vector<uint8_t> mac_addr, string hostname, mutex* stop_m) {

    srand (time(NULL));

    ip = ip_addr;
    mac = mac_addr;
    rx_frame_count = 0;
    name = hostname;
    m_mutex = stop_m;
    frame_generator = new Poisson(POISSON);
    host_start_time = chrono::high_resolution_clock::now();
    frame_queue = new wqueue<MPDU*>;
#ifdef DEBUG
    host_print("Online with MAC " + mac_to_string(mac));
#endif
}


Host::~Host() {
    delete frame_generator;
    delete frame_queue;
}

void Host::arp_test(uint32_t dest_ip, int number_of_arps) {
    // make the threads for receiving and demuxing/sending
    thread receiver_thread(&Host::receiver,this);
    thread mux_demux_thread(&Host::demuxer,this);

    // send some arps
    if (!is_broadcast(dest_ip)) {
#ifdef DEBUG
        host_print("ARPing");
#endif
        arping(dest_ip,number_of_arps,DEFAULT_TIMEOUT);
    }

    // let them run
    receiver_thread.join();
    mux_demux_thread.join();
}

void Host::ping_test(uint32_t destination_ip, int number_of_pings, int delay) {

    // make the threads for receiving and demuxing/sending
    thread receiver_thread(&Host::receiver,this);
    thread mux_demux_thread(&Host::demuxer,this);

    usleep(delay);

    // send some arps
    if (!is_broadcast(destination_ip)) {
        // arping(destination_ip,3,DEFAULT_TIMEOUT);
#ifdef DEBUG
        host_print("Pinging");
#endif
        ping(destination_ip,number_of_pings);


        unique_lock<mutex> lck(*m_mutex);
    }

    // let them run
    receiver_thread.join();
    mux_demux_thread.join();
}

void Host::tcp_test(const char * filename, uint32_t destination_ip, uint16_t source_port, uint16_t destination_port, 
    int is_client) {

    // make the threads for receiving and demuxing/sending
    thread receiver_thread(&Host::receiver,this);
    thread mux_demux_thread(&Host::demuxer,this);

    // send some arps
    if (is_client) {
        // arping(destination_ip,3,DEFAULT_TIMEOUT);
#ifdef L4_DEBUG
        host_print("starting TCP client");
#endif
        

        TCP_client(filename,source_port,destination_ip,destination_port);
    } else {
#ifdef L4_DEBUG
        host_print("starting TCP server");
#endif
        TCP_server(filename,source_port);
    }

    // let them run
    receiver_thread.join();
    mux_demux_thread.join();
}

void Host::udp_test(const char * filename, uint32_t destination_ip, uint16_t source_port, uint16_t destination_port, 
    int is_client) {

    // make the threads for receiving and demuxing/sending
    thread receiver_thread(&Host::receiver,this);
    thread mux_demux_thread(&Host::demuxer,this);

    // send some arps
    if (is_client) {
        // arping(destination_ip,3,DEFAULT_TIMEOUT);
#ifdef L4_DEBUG
        host_print("starting UDP client");
#endif

        UDP_client(filename,source_port,destination_ip,destination_port);
    } else {
#ifdef L4_DEBUG
        host_print("starting UDP server");
#endif
        UDP_server(filename,source_port);
    }

    // let them run
#ifdef L4_DEBUG
        host_print("joining receiver");
#endif
    receiver_thread.join();
#ifdef L4_DEBUG
        host_print("joining mux_demux_thread");
#endif
    mux_demux_thread.join();
}

uint32_t Host::get_ip() {return ip;}

vector<uint8_t> Host::get_mac() {return mac;}

int Host::get_frame_count() {return rx_frame_count;}

void Host::set_ip(uint32_t ip_addr) {
    ip = ip_addr;
}

void Host::set_mac(vector<uint8_t> mac_addr) {
    mac = mac_addr;
}

void Host::connect_ethernet(EthernetLink* e_link, bool flip_wires) {
    // the reason for flipping the wires involved in this link is that if we were to call the same
    // function on both sides of the link without flipping the wires, both ends would be using
    // the same link for transmissions and the same link for receptions, meaning nothing would get through
    if (!flip_wires) {
        tx_interface = e_link->get_wire_1();
        rx_interface = e_link->get_wire_2();
    } else {
        rx_interface = e_link->get_wire_1();
        tx_interface = e_link->get_wire_2();
    }
}

void Host::receiver() {
    // sometimes the processing of frames is delayed, so it is better to just remove the
    // frame from the link and add it to the queue so that the link is cleared and the
    // frame can be processed when the host has available resources

    MPDU* frame;

    // this is to be run in a thread that will wait on a condition variable to check the rx
    // interface.
    while (1) {
        frame = rx_interface->receive();
        usleep((rand() % 1000) + 100);
        frame_queue->add(frame);
#ifdef DEBUG
        host_print(to_string(frame_queue->size()));
#endif
    }

}

void Host::demuxer() {
    // checks the main frame queue and moves MPDUs accordingly.
    // MPDUs whose source MAC address matches the host address 
    // transfered out of the host and all others are processed

    MPDU* frame;
    vector<uint8_t> destination_mac;

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
#ifdef DEBUG
            host_print("process frame");
#endif          
            process_frame(frame, destination_mac);
        }
        
    } 
}

void Host::send_MPDU(MPDU* frame) {

#ifdef DEBUG
    host_print("sending " +to_string(frame->get_size()) + " bytes to " + 
        mac_to_string(frame->get_destination_mac()));
#endif
    usleep((rand() % 1000) + 100);
    // and that frame is sent out on the link
    tx_interface->transmit(frame);
}

void Host::process_frame(MPDU* frame, vector<uint8_t> destination_mac) {
    // the contents of received frames will be processed in this function
    TCP tcp_segment;
    UDP udp_segment;

    increment_frame_count();
       
    // here is where something would be done with the frame (e.g. go to running ping process)
    switch(frame->get_SDU_type()) {
        case MPDU_ARP_TYPE:
            receive_arp(frame->get_SDU());
            delete frame;
            break;

        case MPDU_IP_TYPE: // IP packet  
            // use the protocol of the frame to figure out how to proceed with the contents
            IP packet(frame->get_SDU());
            int socket_found = 0;

            switch(packet.get_protocol()) {
                case IP_PROTOCOL_ICMP:
                    // this is a ping, so send the raw frame to the ICMP socket if it's open
                    for (vector<Socket*>::iterator it = open_ports.begin(); 
                        it != open_ports.end();++it) {

                        // check to see if this is an ICMP socket
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
                case IP_PROTOCOL_TCP:
                    // this is a TCP segment, so retrieve the TCP object
                    // NOTE: make function to determine the port based on the contents of the SDU
                    tcp_segment = generate_TCP(packet.get_SDU());

                    // There may be several flows running
                    // so check to see which socket to use

                    for (vector<Socket*>::iterator it = open_ports.begin(); 
                        it != open_ports.end(); ++it) {

                        // check to see if this port represents a TCP socket and is the same
                        if ((*it)->get_protocol() == IP_PROTOCOL_TCP && (*it)->get_port() == tcp_segment.destination_port) {
                            // send the frame to this port if so
                            (*it)->add_frame(frame);
                        }
                    }
                    break;
                case IP_PROTOCOL_UDP:
                    // this is a TCP segment, so retrieve the UDP object
                    // NOTE: make function to determine the port based on the contents of the SDU
                    udp_segment = generate_UDP(packet.get_SDU());

                    // There may be several flows running
                    // so check to see which socket to use

                    for (vector<Socket*>::iterator it = open_ports.begin(); 
                        it != open_ports.end(); ++it) {

                        // check to see if this port represents a TCP socket and is the same
                        if ((*it)->get_protocol() == IP_PROTOCOL_UDP && (*it)->get_port() == udp_segment.destination_port) {
                            // send the frame to this port if so
                            (*it)->add_frame(frame);
                            break;
                        }
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

    vector<uint8_t> destination_mac;

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

    vector<float> test_times;
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
        chrono::time_point<chrono::high_resolution_clock> ping_send_time = 
            chrono::high_resolution_clock::now();

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
        chrono::duration<double, milli> diff = chrono::high_resolution_clock::now() 
                                                                - ping_send_time;

        test_times.push_back((float)(diff.count()));
#ifdef DEBUG
        host_print("[PING] " + to_string(rx_ip.get_total_length() - rx_ip.get_header_length()) + 
            " bytes from " + mac_to_string(rx_frame->get_source_mac()) + ": icmp_seq=" + 
            to_string(rx_ping.sequence_number) + " time=" + to_string(diff.count()) + " ms");
#endif

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

void Host::print_ping_statistics(int transmissions, int receptions, vector<float> ping_times) {
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
            for (vector<Socket*>::iterator it = open_ports.begin(); it != open_ports.end(); ++it) {
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
    for (vector<Socket*>::iterator it = open_ports.begin(); it != open_ports.end();) {
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


#ifdef DEBUG
    host_print("[ARP] Sending request to " + ip_to_string(requested_ip));
#endif

    send_arp(arp,create_broadcast_mac());
}

void Host::send_arp(ARP tx_arp, vector<uint8_t> destination_mac) {

    // pack it in an IP frame
    MPDU* arp_mpdu = new MPDU;
    arp_mpdu->set_source_mac(mac);
    arp_mpdu->set_destination_mac(destination_mac);
    arp_mpdu->encap_SDU(tx_arp);

    // send it on its merry way
    send_MPDU(arp_mpdu);
}

void Host::receive_arp(vector<uint8_t> arp_u8) {

    
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
#ifdef DEBUG
            host_print("[ARP] Unicast reply from " + ip_to_string(arp_rx.sender_ip) + " [" 
                + mac_to_string(arp_rx.sender_mac) + "]");
#endif
        }
    }
}

void Host::host_print(string statement) {
    cout << setw(15) << name << ": " << statement << endl;
}

void Host::TCP_client(const char* filename, uint16_t this_port, uint32_t dest_ip, uint16_t dest_port) {
    // create socket to send and receive segments
    MPDU * rx_frame, * tx_frame;
    TCP tx_segment, rx_segment, waiting_segment;
    IP rx_packet, tx_packet;
    uint32_t expected_ACK;
    uint32_t rx_SN, tx_SN, expected_SN;   
    bool connection_established = false;
    int bytes_loaded = 0;
    size_t queue_size;
    int triple_ACK_count;
    int cwnd = 1;

    // determine the maximum number of bytes that can be put in the payload and still
    // make it across the network
    int maximum_payload_bytes = MTU - TCP_HEADER_SIZE - IP_HEADER_SIZE - MPDU_HEADER_SIZE;

    /* 
    This is an important object for TCP, since TCP is very much concerned with reliable data transfer.
    This container will house the segments that have to be transfered so that they don't need to be created
    on the fly and dropped packets can be resent quickly.
    */
    vector<TCP> * segments_to_send = file_to_TCP_segments(filename, this_port, dest_port, maximum_payload_bytes);

#ifdef L4_DEBUG
    host_print(to_string(segments_to_send->size()) + " segments created");
#endif

    Socket * TCP_socket = create_socket(this_port, IP_PROTOCOL_TCP);

    // check if the destination MAC address is known and ARP if it is not
    vector<uint8_t> dest_mac = cache.get_mac(dest_ip);

    if (is_broadcast(dest_mac)) {
#ifdef L4_DEBUG
        host_print("[TCP client] No MAC in ARP cache for IP " + ip_to_string(dest_ip) + ". ARPING");
#endif
        // there was no entry, so send an arp to get the mac
        send_request_arp(dest_ip, DEFAULT_TIMEOUT);

        // wait until the mac address has been found and added to the queue
        while (is_broadcast(cache.get_mac(dest_ip))) {
            usleep(100);
        }

        // the mac has been obtained, so get it
        dest_mac = cache.get_mac(dest_ip);
    }

#ifdef L4_DEBUG
    host_print("[TCP client] starting handshake");
#endif

    

    // generate initial sync request
    tx_segment.details = TCP_SYN;
    tx_segment.sequence_number = TCP_INITIAL_SN;
    tx_segment.ACK_number = TCP_INITIAL_SN;
    tx_segment.source_port = this_port;
    tx_segment.destination_port = dest_port;

    while (true) {

        // encap in IP
        tx_packet.encap_SDU(tx_segment);

        // set parameters (these details will remain the same for subsequent sends)
        tx_packet.set_source_ip(ip);
        tx_packet.set_destination_ip(dest_ip);

        //encap in MPDU
        tx_frame = new MPDU();
        // set addressing
        tx_frame->encap_SDU(tx_packet);
        tx_frame->set_destination_mac(dest_mac);
        tx_frame->set_source_mac(mac);

        //send
        send_MPDU(tx_frame);

        // wait for reply
        rx_frame = TCP_socket->get_frame();
        rx_packet = generate_IP(rx_frame->get_SDU());
        rx_segment = generate_TCP(rx_packet.get_SDU()); 

    #ifdef L4_DEBUG
        host_print("[TCP client] got reply");
    #endif

        if ((rx_segment.ACK_number == (tx_segment.sequence_number + 1)) && (rx_segment.details & TCP_SYNACK)) {
            // the ACK number and SN jive and the proper flags are set, so the connection is established
            // as far as the client is concerned

    #ifdef L4_DEBUG
            host_print("[TCP client] connection established");
    #endif

            // send the final ACK necessary to complete the sync on the server side

            tx_segment.details = TCP_ACK; // this is the last segment in the sync
            tx_segment.sequence_number = rx_segment.ACK_number;
            tx_segment.ACK_number = rx_segment.sequence_number + 1;
            // at this point the SN and ACK N should both be 1

            tx_segment.destination_port = dest_port;
            tx_segment.source_port = this_port;

            // encap in IP
            tx_packet.encap_SDU(tx_segment);

            // encap in MPDU
            tx_frame = new MPDU(); // generate a new frame

            // set addressing
            tx_frame->encap_SDU(tx_packet);
            tx_frame->set_destination_mac(dest_mac);
            tx_frame->set_source_mac(mac);

            delete rx_frame; // consume the frame sent by the server

    #ifdef L4_DEBUG
            host_print("[TCP client] sending final sync ACK");
    #endif
            //send ACK
            tx_interface->transmit(tx_frame);

            break;
        } else {
            // keep resending the SYN frame until we get a SYNACK back
            // this will not happen in our case, since nothing should show up at this socket unless
            // the SYN was heard and replied to with a SYNACK
    #ifdef L4_DEBUG
            host_print("[TCP client] did not get proper SYNACK");
    #endif      
            continue;
        }
    }


#ifdef L4_DEBUG
    host_print("[TCP client] handshake successful");
#endif
    int next_segment_index = 0;
    int last_unACKd_index = 0;
    triple_ACK_count = 0; // used to see if the window should be reduced

    while (last_unACKd_index < segments_to_send->size()) {
        if (next_segment_index < segments_to_send->size()) {
            // load the segment
            tx_segment = segments_to_send->at(next_segment_index++);

            // encap in IP
            tx_packet.encap_SDU(tx_segment);

            // encap in MPDU
            tx_frame = new MPDU(); // generate a new frame
            tx_frame->encap_SDU(tx_packet);

            // set addressing
            tx_frame->set_destination_mac(dest_mac);
            tx_frame->set_source_mac(mac);

#ifdef L4_DEBUG
            host_print("[TCP client] sending segment, ACK with " + to_string(tx_segment.sequence_number + 
                tx_segment.payload.size()));
#endif

            send_MPDU(tx_frame);
        }

        // wait for the ack and then send a frame. Note that, because of the loop, a second frame is sent out,
        // which means that the cwnd will double with each new volley of segments

        

        rx_frame = TCP_socket->get_frame();
        rx_packet = generate_IP(rx_frame->get_SDU());
        rx_segment = generate_TCP(rx_packet.get_SDU()); 



        // get the next segment we expected to be ACk'd
        waiting_segment = segments_to_send->at(last_unACKd_index);
        expected_SN = waiting_segment.sequence_number + waiting_segment.payload.size();

        // for the last transmission to have been successful,  the ACK number must be what we expect to use 
        // as the next SN or higher
        if (rx_segment.ACK_number != expected_SN) {
#ifdef L4_DEBUG
            host_print("[TCP client] expected ACK " + to_string(waiting_segment.sequence_number + 
                waiting_segment.payload.size()) + ", got " + to_string(rx_segment.ACK_number));
#endif
            
            if (rx_segment.ACK_number > expected_SN ) {
                // this might not be the expected ACK, but it signifies that previous segments were received
                // properly and so we can update up to which value fo segment was received
                while (true) {
                    waiting_segment = segments_to_send->at(++last_unACKd_index);
                    if (rx_segment.ACK_number ==  waiting_segment.sequence_number + waiting_segment.payload.size()) {
                        last_unACKd_index++;
                        break;
                    }
                }

            } else {
                // NOTE:    figure out how to deal with in-flight ACKs after receiving a triple ACK.
                //          Scenario: have sent out 20 segments, 2nd gets lost, see three ACKs for 
                //          first segment, so drop the cwnd and send out frames again, but there will
                //          still be a ton of ACKs for 2 still coming through, more than enough to drop
                //          cwnd down again

                // check to see if the window should be reduced
                // triple_ACK_count++;

                // // wait for two more ACKs
                // rx_frame = TCP_socket->get_frame();
                // rx_packet = generate_IP(rx_frame->get_SDU());
                // rx_segment = generate_TCP(rx_packet.get_SDU());
                // if (triple_ACK_count == 3) {
                //     if (cwnd > 2) {
                //         cwnd /= 2;
                //     }
                //     triple_ACK_count = 0;
                //     // break;
                // }


                // find the segment at which we should restart
                next_segment_index = last_unACKd_index;

            }


        } else {
            // the segment was successfuly ACK'd, so increase the index for the segment we are waiting to
            // be ACK'd
#ifdef L4_DEBUG
            host_print("[TCP client] segment ACK'd with " + to_string(rx_segment.ACK_number));
#endif
            last_unACKd_index++;
        }

        if ((rx_segment.details & TCP_FIN) && (rx_segment.details & TCP_ACK)) {
            // the final segment has been acked, so we are finished and no longer need to send segments
#ifdef L4_DEBUG
            host_print("[TCP client] final segment ACK'd");
#endif
            break;
        }
            
        if (next_segment_index < segments_to_send->size()) {
            // load the segment
            tx_segment = segments_to_send->at(next_segment_index++);

            // encap in IP
            tx_packet.encap_SDU(tx_segment);

            // encap in MPDU
            tx_frame = new MPDU(); // generate a new frame
            tx_frame->encap_SDU(tx_packet);

            // set addressing
            tx_frame->set_destination_mac(dest_mac);
            tx_frame->set_source_mac(mac);

#ifdef L4_DEBUG
            host_print("[TCP client] sending segment, ACK with " + to_string(tx_segment.sequence_number + 
                tx_segment.payload.size()));
#endif

            send_MPDU(tx_frame);
        }
    }

    delete_socket(TCP_socket->get_port());

    delete segments_to_send;

#ifdef L4_DEBUG
    host_print("[TCP client] finished");
#endif
}

void Host::TCP_server(const char * filename, uint16_t this_port) {
    vector<uint8_t> payload_file; // structure to hold the contents of the file being sent
    size_t payload_size = 0;
    MPDU * rx_frame, * tx_frame;
    TCP tx_segment, rx_segment;
    IP rx_packet, tx_packet;
    uint32_t rx_ACK_n, tx_ACK_n;
    uint32_t last_rx_SN;
    bool connection_established = false;
    int last_payload_size = 0;


    // create a socket to send and receive segments
    Socket * TCP_socket = create_socket(this_port, IP_PROTOCOL_TCP);


    // listening for initial SYN
    rx_frame = TCP_socket->get_frame();
    rx_packet = generate_IP(rx_frame->get_SDU());
    rx_segment = generate_TCP(rx_packet.get_SDU());

#ifdef L4_DEBUG
    host_print("[TCP server] received SYN");
#endif


    while(!(rx_segment.details & TCP_SYN)) {
#ifdef L4_DEBUG
        host_print("[TCP server] invalid SYN");
#endif
        // wait for SYN segment
        delete rx_frame;
        rx_frame = TCP_socket->get_frame();
        rx_packet = generate_IP(rx_frame->get_SDU());
        rx_segment = generate_TCP(rx_packet.get_SDU());
    }

#ifdef L4_DEBUG
    host_print("[TCP server] valid SYN");
#endif


    // host_print("Got SYN segment");

    // SYN received, send SYNACK
    tx_segment.details = TCP_SYNACK;

    tx_segment.sequence_number = TCP_INITIAL_SN;
    tx_segment.ACK_number = rx_segment.sequence_number + 1;

    tx_segment.destination_port = rx_segment.source_port;
    tx_segment.source_port = this_port;

    // encap in IP
    tx_packet.encap_SDU(tx_segment);
    // set parameters (these details will remain the same for subsequent sends)
    tx_packet.set_source_ip(ip);
    tx_packet.set_destination_ip(rx_packet.get_source_ip());

    //encap in MPDU
    tx_frame = new MPDU();
    // set addressing
    tx_frame->encap_SDU(tx_packet);
    tx_frame->set_destination_mac(rx_frame->get_source_mac());
    tx_frame->set_source_mac(mac);

    delete rx_frame;

    //send SYNACK
    send_MPDU(tx_frame);

#ifdef L4_DEBUG
    host_print("[TCP server] SYNACK sent");
#endif


    // upon receiving this frame, the client will have established their connection
    // but the server still needs to wait on a frame with ACK_number = the SN of the segment that was sent + 1

    // listening for final ACK of threeway handshake
    rx_frame = TCP_socket->get_frame();
    rx_packet = generate_IP(rx_frame->get_SDU());
    rx_segment = generate_TCP(rx_packet.get_SDU()); 


    while (!(rx_segment.ACK_number == (tx_segment.sequence_number + 1)) || !(rx_segment.details & TCP_ACK)) {
        // this is not the frame were were expecting, and so the sync is not complete yet
        // so resend the SYNACK (in practice this shouldn't occur)

        //encap in MPDU
        tx_frame = new MPDU();
        // set addressing
        tx_frame->encap_SDU(tx_packet);
        tx_frame->set_destination_mac(rx_frame->get_source_mac());
        tx_frame->set_source_mac(mac);

        delete rx_frame;

        //send SYNACK
        send_MPDU(tx_frame);

    }

    // save this sequence number value for reliable data transfer
    last_rx_SN = rx_segment.sequence_number;

    // the ACK number and SN jive and the proper flags are set, so the connection is established on both ends
    connection_established = true;

    // open an output stream to write to a file
    ofstream ofs;
    ofs.open(filename, ofstream::out);

#ifdef L4_DEBUG
    host_print("[TCP server] connection established");
#endif



    do {
        // listen for segments, send ACKs, build file, wait for FIN bit
        rx_frame = TCP_socket->get_frame();
        rx_packet = generate_IP(rx_frame->get_SDU());
        rx_segment = generate_TCP(rx_packet.get_SDU());

        // for this to be the right segment, the sequence number of the received segment must be equal to the
        // sequence number of the previous segment + the payload size of the previous segment
        if (rx_segment.sequence_number == last_rx_SN + last_payload_size) {
            // confirmed that this is the next expected segment
#ifdef L4_DEBUG
            host_print("[TCP server] got next segment, SN = " + to_string(rx_segment.sequence_number) + " data size = " +
                to_string(rx_segment.payload.size()));
#endif

            // update the last values
            last_rx_SN = rx_segment.sequence_number;
            last_payload_size = rx_segment.payload.size();

            // write payload contents
            for (int i = 0; i < last_payload_size; ++i) {
                ofs.put(rx_segment.payload[i]);
            }

            //prepare ACK
            tx_segment.details = rx_segment.details;
            tx_segment.details |= TCP_ACK;

            tx_segment.sequence_number = rx_segment.ACK_number;
            tx_segment.ACK_number = last_rx_SN + last_payload_size;

            // encap in IP
            tx_packet.encap_SDU(tx_segment);

            // ports should not have changed from last transmission

            //encap in MPDU
            tx_frame = new MPDU();
            // set addressing
            tx_frame->encap_SDU(tx_packet);
            tx_frame->set_destination_mac(rx_frame->get_source_mac());
            tx_frame->set_source_mac(mac);

            delete rx_frame;

#ifdef L4_DEBUG
            host_print("[TCP server] sending ACK " + to_string(tx_segment.ACK_number));
#endif

            send_MPDU(tx_frame);

        } else {
            // error, resend most recent ACK
#ifdef L4_DEBUG
            host_print("[TCP server] error with segment, expected " + to_string(last_rx_SN + last_payload_size) +
                ", got " + to_string(rx_segment.sequence_number));
#endif

            //encap in MPDU the last tx_packet that was created, since that's the ACK for the last successfully
            // received segment
            tx_frame = new MPDU();
            // set addressing
            tx_frame->encap_SDU(tx_packet);
            tx_frame->set_destination_mac(rx_frame->get_source_mac());
            tx_frame->set_source_mac(mac);

#ifdef L4_DEBUG
            host_print("[TCP server] sending ACK " + to_string(tx_segment.ACK_number));
#endif
            send_MPDU(tx_frame);

            delete rx_frame;

        }

        // if the transmitted segment has FIN set, then the client is just waiting for a final ACK
        // and will not longer send segments (unless this segment is lost)
    } while (!(tx_segment.details & TCP_FIN));

#ifdef L4_DEBUG
    host_print("[TCP server] finished receiving, writing to file");
#endif

    // save file
    ofs.close();

    delete_socket(TCP_socket->get_port());

#ifdef L4_DEBUG
    host_print("[TCP server] finished");
#endif

}

void Host::UDP_client(const char* filename, uint16_t this_port, uint32_t dest_ip, uint16_t dest_port) {
    // UDP has no connection, so we just assume that the server is running and fire away with packets.
    // it is the responsibility of higher layers to make sure that the contents arrive without error and
    // that all missing segments are retrieved, so this should be a pretty simple implementation for now

    MPDU * rx_frame, * tx_frame; // we need to delete the rx_frame and have the tx_frame deleted on the other end
    IP rx_packet, tx_packet;
    UDP rx_segment, tx_segment;
    int bytes_loaded = 0;
    int next_segment_index = 0;
    // rx objects will be used later on for requesting missed segments

    Socket * UDP_socket = create_socket(this_port, IP_PROTOCOL_UDP);

    int maximum_payload_bytes = MTU - UDP_HEADER_SIZE - IP_HEADER_SIZE - MPDU_HEADER_SIZE;

    vector<UDP> * segments_to_send = file_to_UDP_segments(filename, this_port, dest_port, maximum_payload_bytes);

#ifdef L4_DEBUG
    host_print(to_string(segments_to_send->size()) + " segments created");
#endif

    // check if the destination MAC address is known and ARP if it is not
    vector<uint8_t> dest_mac = cache.get_mac(dest_ip);

    if (is_broadcast(dest_mac)) {
#ifdef L4_DEBUG
        host_print("[UDP client] No MAC in ARP cache for IP " + ip_to_string(dest_ip) + ". ARPING");
#endif
        // there was no entry, so send an arp to get the mac
        send_request_arp(dest_ip, DEFAULT_TIMEOUT);

        // wait until the mac address has been found and added to the queue
        while (is_broadcast(cache.get_mac(dest_ip))) {
            usleep(100);
        }

        // the mac has been obtained, so get it
        dest_mac = cache.get_mac(dest_ip);
    }

    tx_segment.source_port = this_port;
    tx_segment.destination_port = dest_port;

    // set IP parameters (these details will remain the same for subsequent sends)
    tx_packet.set_source_ip(ip);
    tx_packet.set_destination_ip(dest_ip);

    // not working with checksums yet because bytes are not being corrupted yet


#ifdef L4_DEBUG
    host_print("[UDP client] beginning to send file");
#endif


    while (next_segment_index < segments_to_send->size()) {

        tx_segment = segments_to_send->at(next_segment_index++);


        // encap in IP
        tx_packet.encap_SDU(tx_segment);

        // encap in MPDU
        tx_frame = new MPDU(); // generate a new frame
        tx_frame->encap_SDU(tx_packet);

        // set addressing
        tx_frame->set_destination_mac(dest_mac);
        tx_frame->set_source_mac(mac);

#ifdef L4_DEBUG
        host_print("[UDP client] sending segment " + to_string(next_segment_index));
#endif

        send_MPDU(tx_frame);
    }


#ifdef L4_DEBUG
    host_print("[UDP client] file send complete, deleting socket");
#endif

    delete_socket(UDP_socket->get_port());

}

void Host::UDP_server(const char * filename, uint16_t this_port) {
    // as mentioned with the client, this is more or less a run and receive function that will take incoming packets
    // and form the payload into a file. future implementations could work with go-back-n or selective repeat.
    // additionally, it appears as though a "stream index" is used to differentiate between different UDP flows
    // so the server should probably spawn a new wqueue and thread for each incoming connection in the future

    MPDU * rx_frame, * tx_frame; // we need to delete the rx_frame and have the tx_frame deleted on the other end
    IP rx_packet, tx_packet;
    UDP rx_segment, tx_segment;
    int segment_count = 1;
    // tx objects will be used later on for requesting missed segments

    Socket * UDP_socket = create_socket(this_port, IP_PROTOCOL_UDP);

    // open an output stream to write to a file
    ofstream ofs;
    ofs.open(filename, ofstream::out);

    bool HOST_EOF_reached = false;

#ifdef L4_DEBUG
    host_print("[UDP server] waiting for segments");
#endif

    while (!HOST_EOF_reached) {

        // for the moment, the file will be written when the end of file character is seen
        rx_frame = UDP_socket->get_frame();
#ifdef L4_DEBUG
    host_print("[UDP server] segment received, number " + to_string(segment_count++));
#endif        
        rx_packet = generate_IP(rx_frame->get_SDU());
        rx_segment = generate_UDP(rx_packet.get_SDU());

        if (rx_segment.payload.back() == SEGMENT_EOF) {
            HOST_EOF_reached = true;
            rx_segment.payload.pop_back();
        }

        for (int i = 0; i < rx_segment.payload.size(); ++i) {
            ofs.put(rx_segment.payload[i]);
        }

        delete rx_frame;
    }

#ifdef L4_DEBUG
    host_print("[UDP server] final segment received, writing to file");
#endif

    // save file
    ofs.close();

#ifdef L4_DEBUG
    host_print("[UDP server] file written, closing socket");
#endif

    delete_socket(UDP_socket->get_port());


}