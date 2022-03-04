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

void handshake(int sockfd, struct sockaddr* addr, socklen_t addr_len,
				uint32_t& server_seq_no, uint32_t& server_ack_no, uint16_t& connection_id,
				uint32_t& client_seq_no, uint32_t& client_ack_no, bool* flags)
{
	memset(flags, '\0', NUM_FLAGS);
	// send syn
	unsigned char buf[HEADER_SIZE];
	createHeader(buf, client_seq_no, client_ack_no, connection_id, SYN, flags);

	int length = sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

	cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	// receive syn-ack
	memset(buf, '\0', HEADER_SIZE);	
	memset(flags, '\0', NUM_FLAGS);

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);
	
	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

	cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);
	
	// send ack is completed after handshake
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

	sockaddr* addr = result->ai_addr;
	socklen_t addr_len = result->ai_addrlen;
	// create a UDP socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	uint32_t server_seq_no = INITIAL_SERVER_SEQ;
	uint32_t server_ack_no = 0;
	uint16_t connection_id = 0;

	uint32_t client_seq_no = INITIAL_CLIENT_SEQ;
	uint32_t client_ack_no = 0;

	bool flags[NUM_FLAGS];	// ASF
	
	// Also updates the seq_no, ack_no, conn_id
	handshake(sockfd, addr, addr_len, server_seq_no, server_ack_no, connection_id, client_seq_no, client_ack_no, flags);

	// Finish handshake with payload in ACK sent by client
	// TODO: THIS IS PROBABLY WHERE YOU START A WHILE LOOP SENDING SEGMENTS WITH PAYLOAD AND RECEIVING ACKs
	client_seq_no = server_ack_no;
	client_ack_no = server_seq_no + 1;

	unsigned char buf[MAX_SIZE];
	memset(flags, '\0', NUM_FLAGS);

	createHeader(buf, client_seq_no, client_ack_no, connection_id, ACK, flags);
	// open file to transfer from client to server
	int filefd = open(fileName, O_RDONLY);
	if (filefd == -1) {
		cerr << "ERROR: unable to open file" << endl;
		exit(1);
	}
	struct stat fdStat;
	fstat(filefd, &fdStat);
	size_t bytesRead = 0;
	int counter = 0;
	int length;
	while (counter < fdStat.st_size)
	{
		if (fdStat.st_size - counter > MAX_PAYLOAD_SIZE) {
			bytesRead += read(filefd, buf + HEADER_SIZE, MAX_PAYLOAD_SIZE);
			length = sendto(sockfd, buf, HEADER_SIZE + MAX_PAYLOAD_SIZE, MSG_CONFIRM, addr, addr_len);
		}
		else {
			bytesRead += read(filefd, buf + HEADER_SIZE, fdStat.st_size - counter);
			length = sendto(sockfd, buf, HEADER_SIZE + fdStat.st_size - counter, MSG_CONFIRM, addr, addr_len);
		}
		counter += MAX_PAYLOAD_SIZE;
		printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);	
	}
	cerr << bytesRead << " bytes read from file" << endl;

	// TODO: TO CHECK IF ALL BYTES OF THE FILE HAVE BEEN ACK'D, MAYBE READ THE ENTIRE FILE INTO A BUFFER,
	// GET LENGTH OF FILE, THEN COMPARE LENGTH TO SERVER ACK NO - INITIAL CLIENT SEQ
	// ONCE LENGTH == SERVER ACK NO - INITIAL CLIENT SEQ, THEN START TEARDOWN WITH FIN

	// TODO: the send process and receive process can be put in their own functions

	// 								HEADER_SIZE + payload
	//cerr << "Total bytes sent: " << length << endl;

	memset(buf, '\0', HEADER_SIZE);	
	memset(flags, '\0', NUM_FLAGS);

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);
	
	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

	cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	// sendto(serverSockFd, fileBuffer, bytesRead, MSG_CONFIRM, serverSockAddr, serverSockAddrLength);
	// struct sockaddr addr;
	// socklen_t addr_len = sizeof(struct sockaddr);
	// memset(fileBuffer, 0, sizeof(fileBuffer));
	// ssize_t length = recvfrom(serverSockFd, fileBuffer, fdStat.st_size, 0, &addr, &addr_len);

	close(filefd);
	exit(0);
}

