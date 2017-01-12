#include <stdlib.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <cstdint>
#include <unistd.h>
#include <condition_variable> // condition_variable
#include "host.h"
#include "pdu.h"
#include "addressing.h"
#include "wqueue.h"
#include "data_links.h"
#include "networking_devices.h"

using namespace std;

/*
This file acts as the driver for the network simulator, take a look at the main function for how to operate
either a DHCP, PING, ARP, UDP or TCP test
*/


#define ARP_TEST    1
#define PING_TEST   2
#define DHCP_TEST   3
#define TCP_TEST    4
#define UDP_TEST    5



string name_list[26] = {
    "Alice",
    "Bob",
    "Carol",
    "Daniel",
    "Erica",
    "Frank",
    "Gwen", 
    "Henry",
    "Ida",
    "Joseph",
    "Karla",
    "Liam",
    "Madison",
    "Nate",
    "Ophelia",
    "Phil",
    "Quin",
    "Randy",
    "Sasha",
    "Tim",
    "Ursula",
    "Victor",
    "Wilma",
    "Xavier",
    "Yvette",
    "Zach"
};



void test_print(const char* statement) {
    cout << "\t\t\t\t\t******* ";
    cout << setw(30) << statement;
    cout << " *******" << endl;
}

void test(int test_num, char * if_name, char * of_name) {
    // create the condition variables that will allow us to wait for the completion of the client threads
    // and then shut down the server and switch threads
    mutex * sw_mutex = new mutex;
    mutex * gw_mutex = new mutex;
    mutex * alice_mutex = new mutex;
    mutex * bob_mutex = new mutex;

    // create a switch
    test_print("Creating Switch");
    Switch* test_switch = new Switch("Switch1", sw_mutex);

    test_print("Creating Gateway");
    Gateway* gw = new Gateway("Gateway", gw_mutex, create_random_mac(), create_ip(192,168,0,1));

    uint16_t alice_port = 50000;
    uint16_t bob_port = 30000;

    test_print("Creating MACs");
    vector<uint8_t> alice_mac = create_uniform_mac(0x11);
    vector<uint8_t> bob_mac = create_uniform_mac(0x22);
    // cout << "macs are " << mac_to_string(alice_mac) << " and " << mac_to_string(bob_mac) << endl;

    test_print("Creating IPs");
    uint32_t alice_ip = create_ip(111,111,111,111);
    uint32_t bob_ip = create_ip(222,222,222,222);

    // create two hosts, giving each a distinct ip and mac
    test_print("Creating hosts");
    Host* alice = new Host(alice_mac, "alice", alice_mutex);
    Host* bob = new Host(bob_mac, "bob", alice_mutex);

    // create the two Ethernet links necessary to run the test
    EthernetLink * EL1 = new EthernetLink(true, "A-S"); 
    EthernetLink * EL2 = new EthernetLink(true, "B-S");
    EthernetLink * EL3 = new EthernetLink(true, "GW-S");

    // plug in the hosts to the switch
    test_print("Making ethernet connections");
    alice->connect_ethernet(EL1);
    test_switch->connect_ethernet(EL1,true); // flip the wires to allow for communcications
    bob->connect_ethernet(EL2);
    test_switch->connect_ethernet(EL2,true); // flip the wires
    gw->connect_ethernet(EL3);
    test_switch->connect_ethernet(EL3,true);



    // start the switch thread
    test_print("Starting switch");
    thread* switch_thread = new thread(&Switch::run, test_switch);

    test_print("Starting gateway");
    thread* gw_thread = new thread(&Gateway::run, gw);

    // test certain capabilities of each host
    // uint32_t destination_ip, uint16_t source_port, uint16_t destination_port, int is_client
    test_print("Creating hosts");
    thread* alice_thread, * bob_thread;
    switch(test_num) {
        case ARP_TEST:
            test_print("ARPing ten times");
            alice_thread = new thread(&Host::arp_test, alice, create_broadcast_ip(), 0);
            bob_thread = new thread(&Host::arp_test, bob, alice_ip, 10);
            break;
        case PING_TEST:
            test_print("Pinging ten times");
            alice_thread = new thread(&Host::ping_test, alice,create_broadcast_ip(), 0, 0);
            bob_thread = new thread(&Host::ping_test, bob, alice_ip, 10, 0);
            break;
        case DHCP_TEST:
            test_print("Getting IPs via DHCP");
            alice_thread = new thread(&Host::dhcp_test, alice);
            bob_thread = new thread(&Host::dhcp_test, bob);
            break;
        case TCP_TEST:
            // run alice as the server
            test_print("Sending file via TCP");
            alice_thread = new thread(&Host::tcp_test, alice, of_name, bob_ip, alice_port,bob_port, 0); 
            // run bob as the client
            bob_thread = new thread(&Host::tcp_test, bob, if_name, alice_ip, bob_port, alice_port, 1);
            break;
        case UDP_TEST:
            test_print("Sending file via UDP");
            // run alice as the server
            alice_thread = new thread(&Host::udp_test, alice, of_name, bob_ip, alice_port,bob_port, 0);
            // run bob as the client
            bob_thread = new thread(&Host::udp_test, bob, if_name, alice_ip, bob_port, alice_port, 1);
            break;
    }

    bob_thread->join();
    alice_thread->join();
    switch_thread->join();

    delete switch_thread;
    delete test_switch;
    delete sw_mutex;

    // delete gw_thread;
    // delete gw;
    // delete gw_mutex;

    delete alice_thread;
    delete alice;
    delete alice_mutex;

    delete bob;
    delete bob_thread;
    delete bob_mutex;

    delete EL1;
    delete EL2;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        cout << "Testing requires one argument from [arp, ping, dhcp, tcp, udp]" << endl;
    } else {
        string argv1 = argv[1];
        if (argv1 == "arp") {
            test(ARP_TEST, NULL, NULL);
        } else if (argv1 == "ping") {
            test(PING_TEST, NULL, NULL);
        } else if (argv1 == "dhcp") {
            test(DHCP_TEST, NULL, NULL);
        } else if (argv1 == "tcp") { 
            if (argc != 4) {
                cout << "TCP requires a filename of a file in the same folder and an output filename"  << endl;               
            } else {              
                test(TCP_TEST, argv[2], argv[3]); 
            }
        } else if (argv1 == "udp"){
            if (argc != 4) {
                cout << "UDP requires a filename of a file in the same folder and an output filename"  << endl;               
            } else {              
                test(UDP_TEST, argv[2], argv[3]); 
            }
        } else {
            cout << "Testing requires one argument from [arp, ping, dhcp, tcp, udp]" << endl;
        }
    }

    return 0;
}