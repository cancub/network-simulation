#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <list>
#include "l4_protocols.h"
#include "addressing.h"

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

