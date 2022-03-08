# CS118 Project 2
 
## Makefile
 
This provides a couple make targets for things.
By default (all target), it makes the `server` and `client` executables.
 
It provides a `clean` target, and `tarball` target to create the submission file as well.
 
You will need to modify the `Makefile` to add your userid for the `.tar.gz` turn-in at the top of the file.
 
## Provided Files
 
`server.cpp` and `client.cpp` are the entry points for the server and client part of the project.
 
## Academic Integrity Note
 
You are encouraged to host your code in private repositories on [GitHub](https://github.com/), [GitLab](https://gitlab.com), or other places.  At the same time, you are PROHIBITED to make your code for the class project public during the class or any time after the class.  If you do so, you will be violating academic honestly policy that you have signed, as well as the student code of conduct and be subject to serious sanctions.
 
## Wireshark dissector
 
For debugging purposes, you can use the wireshark dissector from `tcp.lua`. The dissector requires
at least version 1.12.6 of Wireshark with LUA support enabled.
 
To enable the dissector for Wireshark session, use `-X` command line option, specifying the full
path to the `tcp.lua` script:
 
   wireshark -X lua_script:./confundo.lua
 
To dissect tcpdump-recorded file, you can use `-r <pcapfile>` option. For example:
 
   wireshark -X lua_script:./confundo.lua -r confundo.pcap
 
## TEAM
 
Akshay Gupta 305414381
Brendan Rossmango 505370692
Nick Browning 305340620
Cole Strain 005376979

## CONTRIBUTIONS

### Akshay Gupta

Congestion Control, Connection Management, Server Receival and Acknowledgment of Data, Server Port Handling, Git stuff.

### Brendan Rossmango
Client send data/receive ACK and server receive data/send ACK, Handshake and teardown (with 2-second FIN timer), Incorrect port handling, File writing to directory

### Cole Strain
Client teardown with 2-second FIN timer, 10-second abort timer, RTT timer

### Nick Browning
Initial skeleton of server with process header functions, file writing over connection

## HIGH LEVEL DESIGN

### CLIENT DESIGN
The client begins the handshake after establishing connection by sending the SYN, then receiving the SYN-ACK, then sending an ACK with data. 

Then, it reads bytes from the file in a while loop, sending segments to the server and increasing the congestion window size through the slow start and congestion avoidance algorithms. Finally, when the client has sent all bytes of the file and received the last ACK from the server, it then starts the teardown process.

In the teardown process, the client sends a FIN and waits to receive a FIN-ACK from the server. When it receives this FIN-ACK, then it starts a 2 second timer, and during this duration, it responds to any FIN/FIN-ACK with an ACK. When the server receives this ACK from the client, it completes writing the bytes to the specified FILE-DIR. After the 2 second timer ends, the client gracefully terminates connection.

### SERVER DESIGN

The server is designed to break the client-server interactions into 3 parts. One, the handshake, where the connection is established and the SYN, SYN-ACK, ACK are sent. Two, the receiving of the payload from the client. Three, the final handshake to close the connection, where the FIN-ACK is sent when it receives the client’s ACK.

In the first part, the handshake starts with the receiving of a SYN from a client. The server assigns a unique connection ID and creates a file corresponding to this connection to write to.

The second is mainly about receiving the contents of a file from the client. However, it is important to note that the first payload of file contents is received during the handshake. Throughout this phase, the server works to receive the file from the client and writing it. The server handles out-of-order packets being received (though it does not pass the auto-grader for this). The server has a 10 second timer, where if the server does not receive content for 10 seconds, it will abort the connection. This is to prevent a perpetual connection on the server that does nothing but consume resources.

In the final part, the server does its part in closing a connection with the client. It begins this process when it receives a FIN from the client, and works to gracefully terminate the connection. 

## PROBLEMS ENCOUNTERED 

### Writing null bytes to file
Initially, when the server wrote bytes it received from the client to its respective file in the FILE-DIR directory, it did not handle null bytes properly. This is because at first, we wrote the entire payload at once to the correct ofstream using the << operator. This does not work because it stops when it reaches a null byte. To solve this problem, we instead wrote byte by byte of all the bytes in the payload (max size 512). This way, it correctly writes null bytes over the ofstream to the file. 

### Retransmissions
We had trouble with retransmissions as our code often went into an infinite loop sending the same data over and over. Much of the problem came from our own ability to understand how the client should approach understanding the server programmatically, as the client had to figure out the state of the server using ACKs even if the ACKs could be out-of-order or lost. We do not pass the tests for retransmission, however we got our code to a point where the client is able to interact with the reference server and successfully transmit the file over. The way we got to this point was by keeping track of all the ACKs and their acknowledgement numbers that we were waiting to receive. Upon receival of an ACK, we would take that acknowledgment number, and cross reference it with the set. If numbers in this set either match the value received, or were less than the value received, then we would remove that value. This allowed us to avoid extra retransmissions/infinite loops of sending the same data.
