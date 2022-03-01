/* 
Starter code from Tianyuan Yu's Week 7 slides
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

#define MAX_SIZE 524
#define HEADER_SIZE 12
#define MAX_SEQ_ACK_NUMBER 102400
#define RETRANS_TIMEOUT 0.5 
#define INITIAL_CWND 512
#define MAX_CWND 51200
#define RWND 51200
#define INITIAL_SSTHRESH 10000

int main(int argc, char** argv){
	// check if received three command arguments
	if (argc != 4) {
		std::cerr <<"ERROR: Usage: "<< argv[0] << " <HOSTNAME-OR-IP> <PORT> <FILENAME>"<< std::endl;
		exit(1);
	}

	char* hostname = argv[1];
	char* p;
	long portNumber = strtol(argv[2], &p, 10);
	if (*p) {
		std::cerr <<"ERROR: Incorrect port number: "<< argv[2] << std::endl;
		exit(1);
	}
	else {
		if (portNumber < 0 || portNumber > 65536){
			std::cerr <<"ERROR: Incorrect port number: "<< argv[2] << std::endl;
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
		std::cerr << "ERROR: Incorrect hostname" << ret << std::endl;
		exit(1);
	}

	sockaddr* serverSockAddr = result->ai_addr;
	socklen_t serverSockAddrLength = result->ai_addrlen;
	// create a UDP socket
	int serverSockFd = socket(AF_INET, SOCK_DGRAM, 0);
	// open file to transfer from client to server

	int fileToTransferFd = open(fileName, O_RDONLY);
	if (fileToTransferFd == -1) {
		std::cerr << "ERROR: open()" << std::endl;
		exit(1);
	}
	struct stat fdStat;
	fstat(fileToTransferFd, &fdStat);
	uint8_t fileBuffer[fdStat.st_size];
	size_t bytesRead = read(fileToTransferFd, fileBuffer, fdStat.st_size);
	std::cout << bytesRead << " bytes read" << std::endl;

	sendto(serverSockFd, fileBuffer, bytesRead, MSG_CONFIRM, serverSockAddr, 
		serverSockAddrLength);

	std::cout << "DATA sent" << std::endl; 
	struct sockaddr addr;
	socklen_t addr_len = sizeof(struct sockaddr);
	memset(fileBuffer, 0, sizeof(fileBuffer));
	ssize_t length = recvfrom(serverSockFd, fileBuffer, fdStat.st_size, 0, &addr, 
		&addr_len);

	std::string str((char*)fileBuffer);
	std::cerr << "ACK reveived " << length << " bytes: " << std::endl
		<< str << std::endl;
	close(fileToTransferFd);
	exit(0);
}

