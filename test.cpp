#include <stdlib.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <string>
#include "host.h"
#include "pdu.h"
#include "addressing.h"
#include "wqueue.h"
#include "data_links.h"
#include "networking_devices.h"
#include <condition_variable> // condition_variable
#include <list>
#include <cstdint>
#include <unistd.h>

#define ARP_TEST    1
#define PING_TEST   2
#define TCP_TEST    3
#define UDP_TEST    4


using namespace std;

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

// void multiple_switch_test(int number_of_hosts, int number_of_switches) {

//     vector<vector<uint8_t>> mac_addresses;
//     vector<Switch*> switches;
//     vector<Host*> hosts;
//     vector<thread*> switch_threads;
//     vector<thread*> host_threads;
//     uint8_t mac_byte;

//     // start up the router which includes a DHCP server

//     // instantiate the switches and connect them
//     for (int i = 0; i < number_of_switches; i++) {
//         Switch* new_switch = new Switch("Switch" + to_string(i));
//         if (i == 1){
//             cout << "Plugging Switch 1 into Switch 0" << endl;
//             switches.front()->plug_in_device(new_switch);
//         } else if (i >= 2) {
//             int target_switch = rand() % switches.size();
//             switches.at(target_switch)->plug_in_device(new_switch);
//             cout << "Plugging Switch " << i << "into Switch " << target_switch << endl;
//         }
//         switches.push_back(new_switch);
//     }


//     // instantiate the hosts and connect them to the switches at random
//     for (int i = 0; i < number_of_hosts; i++){
//         mac_byte = (uint8_t)i;
//         mac_addresses.push_back(create_uniform_mac(mac_byte));
//         Host* new_host = new Host(create_random_ip(), mac_addresses.back(), name_list[i]);
//         int target_switch = rand() % switches.size();
//         switches.at(target_switch)->plug_in_device(new_host);
//         cout << "Plugging host " << name_list[i] << " into switch " << target_switch << endl;
//         hosts.push_back(new_host);
//     }

//     // wait until the user is ready to start the system
//     int ch = getchar();

//     // prepare the switch threads
//     for (int i = 0; i < number_of_switches; i++) {
//         thread* new_thread = new thread(&Switch::run, switches.at(i));
//         switch_threads.push_back(new_thread);
//     }

//     // prepare the host threads
//     for (int i = 0; i < number_of_hosts; i++) {
//         thread* new_thread = new thread(&Host::run, hosts.at(i), &mac_addresses);
//         host_threads.push_back(new_thread);
//     }

//     // start the switches
//     for (int i = 0; i < number_of_switches; i++) {
//         switch_threads.at(i)->join();
//     }

//     // start the hosts
//     for (int i = 0; i < number_of_hosts; i++) {
//         host_threads.at(i)->join();
//     }

//     // kill the hosts
//     while (hosts.size() > 0) {
//         host_threads.pop_back();
//         hosts.pop_back();
//     }

//     // kill the switches
//     while (switches.size() > 0) {
//         switch_threads.pop_back();
//         switches.pop_back();
//     }

//     // kill the router


// }

// void arp_test() {
//     // create a switch
//     Switch* test_switch = new Switch("Switch1");

//     cout << "creating MACs" << endl;
//     vector<uint8_t> alice_mac = create_uniform_mac(0x11);
//     vector<uint8_t> bob_mac = create_uniform_mac(0x22);
//     // cout << "macs are " << mac_to_string(alice_mac) << " and " << mac_to_string(bob_mac) << endl;

//     cout << "creating IPs" << endl;
//     uint32_t alice_ip = create_ip(111,111,111,111);
//     uint32_t bob_ip = create_ip(222,222,222,222);

//     // create two hosts, giving each a distinct ip and mac
//     Host* alice = new Host(alice_ip, alice_mac, "alice");
//     Host* bob = new Host(bob_ip, bob_mac, "bob");

//     // plug in the hosts to the switch
//     cout << "plugging in hosts" << endl;
//     test_switch->plug_in_device(alice);
//     test_switch->plug_in_device(bob);


//     // start the switch thread
//     cout << "starting switch" << endl;
//     thread* switch_thread = new thread(&Switch::run, test_switch);

//     // test the arp capabilities of each host
//     cout << "starting hosts" << endl;
//     thread* alice_thread = new thread(&Host::run, alice, bob_ip,0);
//     thread* bob_thread = new thread(&Host::run, bob, alice_ip, 250000);


//     switch_thread->join();
//     alice_thread->join();
//     bob_thread->join();

// }

void test_print(const char* statement) {
    cout << "\t\t\t\t\t******* ";
    cout << setw(30) << statement;
    cout << " *******" << endl;
}

void test(int test_num, char * if_name, char * of_name) {
    // create the condition variables that will allow us to wait for the completion of the client threads
    // and then shut down the server and switch threads
    mutex * sw_mutex = new mutex;
    mutex * alice_mutex = new mutex;
    mutex * bob_mutex = new mutex;

    // create a switch
    test_print("Creating Switch");
    Switch* test_switch = new Switch("Switch1", sw_mutex);

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
    Host* alice = new Host(alice_ip, alice_mac, "alice", alice_mutex);
    Host* bob = new Host(bob_ip, bob_mac, "bob", alice_mutex);

    // create the two Ethernet links necessary to run the test
    EthernetLink * EL1 = new EthernetLink(true, "A-S"); //monitor the interface between alice and switch
    EthernetLink * EL2 = new EthernetLink(false, "B-S");

    // plug in the hosts to the switch
    test_print("Plugging in hosts");
    alice->connect_ethernet(EL1);
    test_switch->connect_ethernet(EL1,true); // flip the wires to allow for communcications
    bob->connect_ethernet(EL2);
    test_switch->connect_ethernet(EL2,true); // flip the wires


    // start the switch thread
    test_print("Starting switch");
    thread* switch_thread = new thread(&Switch::run, test_switch);

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
        cout << "Testing requires one argument from [arp, ping, tcp, udp]" << endl;
    } else {
        string argv1 = argv[1];
        if (argv1 == "arp") {
            test(ARP_TEST, NULL, NULL);
        } else if (argv1 == "ping") {
            test(PING_TEST, NULL, NULL);
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
            cout << "Testing requires one argument from [arp, ping, tcp, udp]" << endl;
        }
    }

    return 0;
}