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

using namespace std;

uint32_t seq_no;
uint32_t ack_no;
uint16_t connection_id;

void createHeader(char* head, uint32_t seq, uint32_t ack, uint16_t conn_id, int flag){
    head[0] = (seq >> 24) & 0Xff;
    head[1] = (seq >> 16) & 0Xff;
    head[2] = (seq >> 8) & 0Xff;
    head[3] = (seq >> 0) & 0Xff;
    head[4] = (ack >> 24);
    head[5] = (ack >> 16);
    head[6] = (ack >> 8);
    head[7] = (ack >> 0);
    head[8] = 0x00;
	if (conn_id == 1){
		head[9] = 0x01;
	}
	else head[9] = 0x00;
    head[10] = 0x00;
	if (flag == 1){
		head[11] = 0x02;
	}
}

void handshake(int sockfd, struct sockaddr* addr, socklen_t addr_len){
	// send syn
	char syn_buf[HEADER_SIZE];
	createHeader(syn_buf, INITIAL_CLIENT_SEQ, 0, 0, 1);

	int length = sendto(sockfd, syn_buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

	cerr << "Total bytes sent: " << length << endl;
	cout << "SEND " << INITIAL_CLIENT_SEQ << " 0 0 " << INITIAL_CWND << " " << INITIAL_SSTHRESH << " SYN" << endl;

	// receive syn-ack
	memset(syn_buf, 0, HEADER_SIZE);
	length = recvfrom(sockfd, syn_buf, HEADER_SIZE, 0, addr, &addr_len);
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

