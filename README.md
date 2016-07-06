host -- link -- host (COMPLETE)

host -- link -- switch -- link -- host

host -- link -- switch -- link -- host
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


--------------------------------------------------------------------------------
CURRENT WORK:

- should probably implement an interface class that can be inherited by hosts, switches, routers
	for their own purposes

- expose link object in nodes, with each link containing a mutex that must be 
	given to each node attached to it

-need to implement new paradigm for communicating frames:

	one thread waits on reception condition variable
		- upon setting of variable, takes lock
		- prints out contents of frame
		- sets contents to 0
		- unlocks
	another thread for sending
		- wait until time is up for sending a frame
		- take the lock
		- put frame on link
		- release lock
		- set transmission condition variable

	threads are independent (full duplex)

	at switch:
	one thread for each reception condition variable
		- upon setting of variable, takes lock
		- two options:
			- copy frame, unlock, notify other end of link, get queue mutex, put in queue, release lock,
				notify all threads trying to access queue
			- get queue mutex, move frame directly from the interface to the main queue,
				release both mutexes and notify all interested parties for both

--------------------------------------------------------------------------------
FUTURE WORK:

- implement routing protocols, network discovery protocols for switch to figure out ports
- have frame sent from one specific host to another
- multicast
- arp
- ping
- tcp
- introduce error for force retransmit
- make wireless link class

