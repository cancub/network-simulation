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


--------------------------------------------------------------------------------
CURRENT WORK:

- switch ip and mac addresses from string to array of uint8_t

- begin to build up network layer
	- stations must obtain an IP
	- set up basic ip frames such as bootp, icmp, arp

- create a gateway device
	- provides ip addresses to other devices in network
	- can have whitelist

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

