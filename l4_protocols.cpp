#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <list>
#include "l4_protocols.h"
#include "l3_protocols.h"
#include "pdu.h"
#include "addressing.h"
#include "data_links.h"

using namespace std;

UDP generate_UDP(vector<uint8_t> udp_u8) {

    UDP udp_result;

    // uint16_t source_port;
    udp_result.source_port = (((uint16_t)(udp_u8[0]) << 8) & 0xFF00) + (udp_u8[1]);

    // uint16_t destination_port;
    udp_result.destination_port = (((uint16_t)(udp_u8[2]) << 8) & 0xFF00) + (udp_u8[3]);

    // uint16_t length;
    udp_result.length = (((uint16_t)(udp_u8[4]) << 8) & 0xFF00) + (udp_u8[5]);

    // uint16_t checksum;
    udp_result.checksum = (((uint16_t)(udp_u8[6]) << 8) & 0xFF00) + (udp_u8[7]);

    // std::vector<uint8_t> payload;
    udp_result.payload.reserve(udp_u8.size() - UDP_HEADER_SIZE);
    for (int i = UDP_HEADER_SIZE; i < udp_u8.size(); i++) {
        udp_result.payload.push_back(udp_u8[i]);
    }
    
    return udp_result;
}

vector<UDP> * file_to_UDP_segments(const char * filename, uint16_t src_port, uint16_t dest_port, int maximum_bytes) {

    // take a file, represented as a vector of bytes, a souce and destination port,
    // and create all the segments which will have to be sent out
    
    vector<UDP> * result = new vector<UDP>();

    vector<uint8_t> file;
    ifstream ifs;
    ifs.open (filename, ifstream::in);
    char c = ifs.get();

    while (ifs.good()) {
        file.push_back(c);

        c = ifs.get();
    }

    ifs.close();

    int bytes_loaded = 0;

    // cout << "number of segments = " << ((file.size() / maximum_bytes) + 1) << endl;

    UDP tx_segment;
    tx_segment.source_port = src_port;
    tx_segment.destination_port = dest_port;

    tx_segment.payload.clear();
    // prep the payload by reserving however many bytes are needed
    if (file.size() - bytes_loaded > maximum_bytes) {
        tx_segment.payload.reserve(maximum_bytes);
    } else {
        tx_segment.payload.reserve(file.size() - bytes_loaded);        
    }

    // load bytes into the payload
    while (true) {
        // load a byte into the payload
        tx_segment.payload.push_back(file[bytes_loaded++]);
        // go until either the maximum number of bytes have been loaded or the last byte of the
        // file has been loaded
        if (bytes_loaded == file.size()) {
            // this will be the last segment sent, because we have run out of data to send
            tx_segment.payload.push_back(SEGMENT_EOF);
            result->push_back(tx_segment);
            break;
        } else if (tx_segment.payload.size() == maximum_bytes) {
            // there will be more data to send, but there's no more room in the segment to send it
            result->push_back(tx_segment);
            tx_segment.payload.clear();
        }
    }

    return result;
}

void UDP_client(const char* filename, uint16_t this_port, uint32_t dest_ip, uint16_t dest_port, Socket * UDP_socket,
    EthernetWire* tx_interface) {
    // UDP has no connection, so we just assume that the server is running and fire away with packets.
    // it is the responsibility of higher layers to make sure that the contents arrive without error and
    // that all missing segments are retrieved, so this should be a pretty simple implementation for now

    MPDU * rx_frame, * tx_frame; // we need to delete the rx_frame and have the tx_frame deleted on the other end
    IP rx_packet, tx_packet;
    UDP rx_segment, tx_segment;
    int bytes_loaded = 0;
    int next_segment_index = 0;
    // rx objects will be used later on for requesting missed segments

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

        usleep((rand() % 1000) + 100);
        // and that frame is sent out on the link
        tx_interface->transmit(frame);
    }


#ifdef L4_DEBUG
    host_print("[UDP client] file send complete, deleting socket");
#endif


}

void UDP_server(const char * filename, uint16_t this_port, Socket * UDP_socket, EthernetWire* tx_interface) {
    // as mentioned with the client, this is more or less a run and receive function that will take incoming packets
    // and form the payload into a file. future implementations could work with go-back-n or selective repeat.
    // additionally, it appears as though a "stream index" is used to differentiate between different UDP flows
    // so the server should probably spawn a new wqueue and thread for each incoming connection in the future

    MPDU * rx_frame, * tx_frame; // we need to delete the rx_frame and have the tx_frame deleted on the other end
    IP rx_packet, tx_packet;
    UDP rx_segment, tx_segment;
    int segment_count = 1;
    // tx objects will be used later on for requesting missed segments

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

}


// TCPSegmentList::TCPSegmentList() {segment_list = new list<TCP>;}
// TCPSegmentList::~TCPSegmentList() {delete segment_list;}

// void TCPSegmentList::add(TCP new_segment) {
//     //check to see if the segment isn't already in the list
//     bool segment_exists = false;
//     for (list<TCP>::iterator it = segment_list->begin(); it != segment_list->end(); ++it) {
//         if((*it).sequence_number == new_segment.sequence_number) {
//             // segment already exists, so don't add it 
//             segment_exists = true;
//             break;
//         }
//     }

//     // add to the end of the list if it's not already present
//     if (!segment_exists) {
//         segment_list->push_back(new_segment);
//     }
// }


// void TCPSegmentList::remove(TCP segment_to_remove) {
//     // find the first matching segment and remove it from the list
//     // NOTE: write a predicate and use remove_if. As it stands, this runs double the necessary operations

//     int index = 0;
//     bool element_found = false;

//     for (list<TCP>::iterator it = segment_list->begin(); it != segment_list->end(); ++it) {
//         if ((*it).sequence_number == segment_to_remove.sequence_number) {
//             element_found = true;
//             break;
//         }
//         index++;
//     }

//     segment_list->erase(index);
    
// }


TCP generate_TCP(vector<uint8_t> tcp_u8) {

    TCP tcp_result;

    // uint16_t source_port;
    tcp_result.source_port = (((uint16_t)(tcp_u8[0]) << 8) & 0xFF00) + (tcp_u8[1]);

    // uint16_t destination_port;
    tcp_result.destination_port = (((uint16_t)(tcp_u8[2]) << 8) & 0xFF00) + (tcp_u8[3]);

    // uint32_t sequence_number;
    tcp_result.sequence_number = (((uint32_t)(tcp_u8[4]) << 24) & 0xFF000000) + 
                                    (((uint32_t)(tcp_u8[5]) << 16) & 0x00FF0000) +
                                    (((uint32_t)(tcp_u8[6]) << 8) & 0x0000FF00) +
                                    (((uint32_t)(tcp_u8[7])) & 0x000000FF);

    // uint32_t ACK_number;
    tcp_result.ACK_number = (((uint32_t)(tcp_u8[8]) << 24) & 0xFF000000) + \
                                    (((uint32_t)(tcp_u8[9]) << 16) & 0x00FF0000) + \
                                    (((uint32_t)(tcp_u8[10]) << 8) & 0x0000FF00) +
                                    (((uint32_t)(tcp_u8[11])) & 0x000000FF);

    // uint16_t details;
    tcp_result.details = (((uint16_t)(tcp_u8[12]) << 8) & 0xFF00) + (tcp_u8[13]);

    // uint16_t receive_window;
    tcp_result.receive_window = (((uint16_t)(tcp_u8[14]) << 8) & 0xFF00) + (tcp_u8[15]);

    // uint16_t checksum;
    tcp_result.checksum = (((uint16_t)(tcp_u8[16]) << 8) & 0xFF00) + (tcp_u8[17]);

    // std::vector<uint8_t> payload;
    // total size of header at this point is 18 bytes
    tcp_result.payload.reserve(tcp_u8.size() - TCP_HEADER_SIZE);
    for (int i = TCP_HEADER_SIZE; i < tcp_u8.size(); i++) {
        tcp_result.payload.push_back(tcp_u8[i]);
    }
    
    return tcp_result;
}

vector<TCP> * file_to_TCP_segments(const char * filename, uint16_t src_port, uint16_t dest_port, int maximum_bytes) {

    // take a file, represented as a vector of bytes, a souce and destination port,
    // and create all the segments which will have to be sent out
    
    vector<TCP> * result = new vector<TCP>();

    vector<uint8_t> file;
    ifstream ifs;
    ifs.open (filename, ifstream::in);
    char c = ifs.get();

    while (ifs.good()) {
        file.push_back(c);

        c = ifs.get();
    }

    ifs.close();

    int bytes_loaded = 0;

    TCP tx_segment;
    tx_segment.source_port = src_port;
    tx_segment.destination_port = dest_port;

    tx_segment.sequence_number = 1; // these values for SN and ACK_N come from a successful three-way handshake
    tx_segment.ACK_number = 1;
    tx_segment.details = TCP_ACK;

    while (!(tx_segment.details & TCP_FIN)) {

        tx_segment.payload.clear();
        // prep the payload by reserving however many bytes are needed
        if (file.size() - bytes_loaded > maximum_bytes) {
            tx_segment.payload.reserve(maximum_bytes);
        } else {
            tx_segment.payload.reserve(file.size() - bytes_loaded);        
        }

        // load bytes into the payload
        while (true) {
            // load a byte into the payload
            tx_segment.payload.push_back(file[bytes_loaded++]);
            // go until either the maximum number of bytes have been loaded or the last byte of the
            // file has been loaded
            if (bytes_loaded == file.size()) {
                // this will be the last segment sent, because we have run out of data to send
                tx_segment.details |= TCP_FIN;

                break;
            } else if (tx_segment.payload.size() == maximum_bytes) {
                // there will be more data to send, but there's no more room in the segment to send it
                break;
            }
        }

        if (result->size() > 0) {
            tx_segment.sequence_number = result->back().sequence_number + result->back().payload.size();
        }

        result->push_back(tx_segment);
    }

    return result;
}

void TCP_client(const char* filename, uint16_t this_port, uint32_t dest_ip, uint16_t dest_port, Socket* TCP_socket,
    EthernetWire* tx_interface) {
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
        usleep((rand() % 1000) + 100);
        // and that frame is sent out on the link
        tx_interface->transmit(frame);

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

            usleep((rand() % 1000) + 100);
            // and that frame is sent out on the link
            tx_interface->transmit(frame);
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

            usleep((rand() % 1000) + 100);
            // and that frame is sent out on the link
            tx_interface->transmit(frame);
        }
    }

    delete segments_to_send;

#ifdef L4_DEBUG
    host_print("[TCP client] finished");
#endif
}

void TCP_server(const char * filename, uint16_t this_port, Socket * TCP_socket, EthernetWire * tx_interface) {
    vector<uint8_t> payload_file; // structure to hold the contents of the file being sent
    size_t payload_size = 0;
    MPDU * rx_frame, * tx_frame;
    TCP tx_segment, rx_segment;
    IP rx_packet, tx_packet;
    uint32_t rx_ACK_n, tx_ACK_n;
    uint32_t last_rx_SN;
    bool connection_established = false;
    int last_payload_size = 0;


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
    usleep((rand() % 1000) + 100);
    // and that frame is sent out on the link
    tx_interface->transmit(frame);

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
        usleep((rand() % 1000) + 100);
        // and that frame is sent out on the link
        tx_interface->transmit(frame);

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

            usleep((rand() % 1000) + 100);
            // and that frame is sent out on the link
            tx_interface->transmit(frame);

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
            usleep((rand() % 1000) + 100);
            // and that frame is sent out on the link
            tx_interface->transmit(frame);

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

#ifdef L4_DEBUG
    host_print("[TCP server] finished");
#endif

}