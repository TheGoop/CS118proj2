/* 
Starter code from Tianyuan Yu's Week 7 slides
*/

/*
Client: "SEND" <Sequence Number> <Acknowledgement Number> <Connection ID> <CWND> <SS-THRESH> ["ACK"] ["SYN"] ["FIN"] ["DUP"]
Client: "RECV" <Sequence Number> <Acknowledgement Number> <Connection ID> <CWND> <SS-THRESH> ["ACK"] ["SYN"] ["FIN"]
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <list>
#include <sys/stat.h>
#include <time.h>

#include "constants.h"
#include "utils.h"

using namespace std;

void handshake(int sockfd, struct sockaddr* addr, socklen_t addr_len){
	// send syn
	uint32_t seq_no = INITIAL_CLIENT_SEQ;
	uint32_t ack_no = 0;
	uint16_t connection_id = 0;
	bool flags[3];	// ASF
	memset(flags, '\0', 3);

	unsigned char syn_buf[HEADER_SIZE];
	createHeader(syn_buf, seq_no, ack_no, connection_id, SYN, flags);

	int length = sendto(sockfd, syn_buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

	cerr << "Total bytes sent: " << length << endl;
	cout << "SEND " << seq_no << " " << ack_no << " " << connection_id << " " << INITIAL_CWND << " " << INITIAL_SSTHRESH;
	if (flags[0]) std::cout << " ACK";
	if (flags[1]) std::cout << " SYN";
	if (flags[2]) std::cout << " FIN";
	std::cout << std::endl;

	// receive syn-ack
	unsigned char buf[HEADER_SIZE];
	memset(flags, '\0', 3);

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);
	
	processHeader(buf, seq_no, ack_no, connection_id, flags);

	cerr << "Total bytes received: " << length << endl;
	cout << "RECV " << seq_no << " " << ack_no << " " << connection_id << " " << 
		INITIAL_CWND << " " << INITIAL_SSTHRESH;
	if (flags[0]) std::cout << " ACK";
	if (flags[1]) std::cout << " SYN";
	if (flags[2]) std::cout << " FIN";
	std::cout << std::endl;

	// send ack


}

int main(int argc, char** argv){
	// Check if number of args is correct
	if (argc != 4) {
		cerr <<"ERROR: Usage: ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>"<< endl;
		exit(1);
	}

	char* hostname = argv[1];
	// Check for invalid port number
	char* p;
	long portNumber = strtol(argv[2], &p, 10); // convert string to number, base 10
	if (*p) {
		cerr << "ERROR: Incorrect port number: "<< argv[2] << endl;
		exit(1);
	}
	else {
		if (portNumber < 0 || portNumber > 65536){
			cerr <<"ERROR: Incorrect port number: "<< argv[2] << endl;
			exit(1);
		}
	}
	char* fileName = argv[3];
	// set the hints for getaddrinfo()
	struct addrinfo hints;
	struct addrinfo* result;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;  // UDP socket
	hints.ai_family = AF_INET;  // IPv4 
	// get server address info using hints
	int ret;
	if ((ret = getaddrinfo(hostname, argv[2], &hints, &result)) != 0){
		cerr << "ERROR: Incorrect hostname or IP" << ret << endl;
		exit(1);
	}

	sockaddr* serverSockAddr = result->ai_addr;
	socklen_t serverSockAddrLength = result->ai_addrlen;
	// create a UDP socket
	int serverSockFd = socket(AF_INET, SOCK_DGRAM, 0);

	handshake(serverSockFd, serverSockAddr, serverSockAddrLength);

	// char buf[524];
	// memset(buf, '\0', 524);
	// struct sockaddr addr;
	// socklen_t addr_len = sizeof(struct sockaddr);

	// ssize_t length = recvfrom(serverSockFd, buf, 524, 0, &addr, &addr_len);
	// std::cerr << "DATA received " << length << " bytes from : " << inet_ntoa(((struct sockaddr_in*) & addr)->sin_addr) << std::endl;

	// open file to transfer from client to server
	// int fileToTransferFd = open(fileName, O_RDONLY);
	// if (fileToTransferFd == -1) {
	// 	cerr << "ERROR: unable to open file" << endl;
	// 	exit(1);
	// }
	// struct stat fdStat;
	// fstat(fileToTransferFd, &fdStat);
	// uint8_t fileBuffer[fdStat.st_size];
	// size_t bytesRead = read(fileToTransferFd, fileBuffer, fdStat.st_size);
	// cerr << bytesRead << " bytes read" << endl;
	// sendto(serverSockFd, fileBuffer, bytesRead, MSG_CONFIRM, serverSockAddr, serverSockAddrLength);

	// struct sockaddr addr;
	// socklen_t addr_len = sizeof(struct sockaddr);
	// memset(fileBuffer, 0, sizeof(fileBuffer));
	// ssize_t length = recvfrom(serverSockFd, fileBuffer, fdStat.st_size, 0, &addr, &addr_len);

	// string str((char*)fileBuffer);
	// cerr << "ACK received " << length << " bytes: " << endl << str << endl;
	// close(fileToTransferFd);
	// exit(0);
}

