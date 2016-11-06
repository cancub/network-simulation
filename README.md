host -- link -- host (COMPLETE)

host -- link -- switch -- link -- host (COMPLETE)

host -- link -- switch -- link -- switch -- host (COMPLETE)
				   |
				  link
				   |
				  host



				gateway
			       |
				   |
host -- link -- switch -- link -- host
				   |
				  link
				   |
				  host

--------------------------------------------------------------------------------
COMPLETED WORK:

Data link layer:
- make frame sending process
- make host which uses frame sending process
- communicate frame between two hosts seperated by a switch
- standardized interface class with thread safe queue
- every network device sees an interface as a combination of a rx and tx interface,
  each with its own mutex and queue
- switch object has many rx threads and one tx thread which operate independently and without
  collision (full duplex)
- same for hosts, obviously
- hosts can be plugged into each other or a switch, switches can be plugged into each other
- frames can be copied in case they need to be broadcast to many ports
- frames are created and consumed within the network, reference to frame object gets passed around
- switch ip and mac addresses from string to array of uint8_t
- host has a wqueue for each process to add to
- host has a mux/demux thread for sending frames and passing incoming frames to the proper process(es)

network layer:


--------------------------------------------------------------------------------
CURRENT WORK:

STOPPED AT: 



- begin to build up network layer
	- add ports, creating a queue for each port so that a process in the application
		layer can send and receive their own queue
		- the ports list should be a thread-safe vector of port objects that starts off empty
		- port objects are comprised of:
			- port number
			- process name
			- thread safe queue that both the process and the main demuxer can reference
		- each application, as it starts, does the following:
			- waits on a mutex for the vector
			- checks to see if its desired port number is taken by scanning the ports in the array
			- if it isn't taken, it inserts a port object at the sorted position in the array
			- if it is taken, it find the next highest port it can take and does the above
		- this way, when a packet arrives, the demuxer can look at the port number and add the underlying
			PDU to the queue for the process and also scan through the open ports to see if any of these
			queues have frames to send

	- start off by assuming that hosts already have mac address and IP
	- add functionality for encaspulation of network layer PDU as MAC layer PSU

	- stations must obtain an IP
	- set up basic ip frames such as bootp, icmp, arp
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

	- encapsulation and de-encapsulation:
		- when a frame is received, the first thing that the host's demuxer will do is see
			if it should process the frame
		- if it should process the frame, it looks at the type of the PSU to see how it will be processed
		- at this point we can assume that the PSU is an ARP or IP packet and thus there should be a constructor
			that takes an array of u8 (the PSU) and converts it into an IP packet
		- the host then looks at the resulting  

--------------------------------------------------------------------------------
FUTURE WORK:

- implement routing protocols, network discovery protocols for switch to figure out ports
- throughput and dropped frames monitoring
- make frame size based on actual frame contents
	- have it calculated at time of sending
- multicast
- tcp
- introduce error for force retransmit
- make wireless link class

