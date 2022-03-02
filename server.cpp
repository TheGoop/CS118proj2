/* 
Starter code from Tianyuan Yu's Week 7 slides
*/

/*
Server: "RECV" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"]
Server: "SEND" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"] ["DUP"]
*/

#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <inttypes.h>
#include <cstring>

#include "constants.h"
#include "packet.h"
#include "utils.h"

using namespace std;

uint32_t currSeq;
uint32_t currAck;
uint16_t currID;
bool flags[3];

void processHeader(unsigned char *buf) {
    currSeq = (buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);

    currAck = (buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7]);

    currID = (buf[8] << 8 | buf[9]);

    flags[0] = (buf[11] >> 2) & 1;
    flags[1] = (buf[11] >> 1) & 1;
    flags[2] = (buf[11] >> 0) & 1;

    cerr << currSeq << endl;
    cerr << currAck << endl;
    cerr << currID << endl;
    cerr << flags[0] << flags[1] << flags[2] << endl;
}

void printHeader(unsigned char *buf) {
	cerr << ((buf[0] >> 31) & 1) << endl;
	cerr << ((buf[0] >> 30) & 1) << endl;
	cerr << ((buf[0] >> 29) & 1) << endl;
	cerr << ((buf[0] >> 28) & 1) << endl;
	cerr << ((buf[0] >> 27) & 1) << endl;
	cerr << ((buf[0] >> 26) & 1) << endl;
	cerr << ((buf[0] >> 25) & 1) << endl;
	cerr << ((buf[0] >> 24) & 1) << endl;
	cerr << ((buf[0] >> 23) & 1) << endl;
	cerr << ((buf[0] >> 22) & 1) << endl;
	cerr << ((buf[0] >> 21) & 1) << endl;
	cerr << ((buf[0] >> 20) & 1) << endl;
	cerr << ((buf[0] >> 19) & 1) << endl;
	cerr << ((buf[0] >> 18) & 1) << endl;
	cerr << ((buf[0] >> 17) & 1) << endl;
	cerr << ((buf[0] >> 16) & 1) << endl;
	cerr << ((buf[0] >> 15) & 1) << endl;
	cerr << ((buf[0] >> 14) & 1) << endl;
	cerr << ((buf[0] >> 13) & 1) << endl;
	cerr << ((buf[0] >> 12) & 1) << endl;
	cerr << ((buf[0] >> 11) & 1) << endl;
	cerr << ((buf[0] >> 10) & 1) << endl;
	cerr << ((buf[0] >> 9) & 1) << endl;
	cerr << ((buf[0] >> 8) & 1) << endl;
	cerr << ((buf[0] >> 7) & 1) << endl;
	cerr << ((buf[0] >> 6) & 1) << endl;
	cerr << ((buf[0] >> 5) & 1) << endl;
	cerr << ((buf[0] >> 4) & 1) << endl;
	cerr << ((buf[0] >> 3) & 1) << endl;
	cerr << ((buf[0] >> 2) & 1) << endl;
	cerr << ((buf[0] >> 1) & 1) << endl;
}

int main(int argc, char** argv){
	//check arguments
	if(argc != 2){
		cerr <<"ERROR: Usage: "<< argv[0] << " <PORT> "<< endl;
		exit(1);
	}
	char* p;
	long portNumber = strtol(argv[1], &p, 10);	// convert string to number, base 10
	if (*p) {
		cerr <<"ERROR: Incorrect port number: "<< argv[1] << endl;
		exit(1);
	}
	else {
		if (portNumber < 0 || portNumber > 65536){
			cerr <<"ERROR: Incorrect port number: "<< argv[1] << endl;
			exit(1);
		}
	}
	
	//UDP socket
	int serverSockFd = socket(AF_INET, SOCK_DGRAM, 0);
	struct addrinfo hints;
	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo* myAddrInfo;
	int ret;
	if ((ret = getaddrinfo(NULL, argv[1], &hints, &myAddrInfo)) != 0){
		cerr << "ERROR: getaddrinfo" << endl;
		exit(1);
	}
	if (bind(serverSockFd, myAddrInfo->ai_addr, myAddrInfo->ai_addrlen) == -1){
		cerr << "ERROR: bind" << endl;
		exit(1);
	}
	Packet packet;
	while (1){
		char buf[1024]; //extra space to be safe
		struct sockaddr addr;
		socklen_t addr_len = sizeof(struct sockaddr);
		int bytes = recvPacket(serverSockFd, packet, &addr, &addr_len);
		// ssize_t length = recvfrom(serverSockFd, buf, 1024, 0, &addr, &addr_len);
		char* client_ip = inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr);
		cerr << "DATA received " << bytes << " bytes from: " << client_ip << endl;
		cout << "RECV " << packet.getSeqNo() << " " << packet.getAckNo() << " " << packet.getConnectionID() 
			<< " " << INITIAL_CWND << " " << INITIAL_SSTHRESH; 		
		if (packet.getSYN())
			cout << " SYN" << endl;

		processHeader((unsigned char *) &packet);
		printHeader((unsigned char *) &packet);

		// bytes = sendto(serverSockFd, "ACK", strlen("ACK"), MSG_CONFIRM, &addr, addr_len);
		// cerr << bytes << " bytes ACK sent" << endl; 
	}
	exit(0);
}