/* 
Starter code from Tianyuan Yu's Week 7 slides
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
#include <thread>
#include <fstream>
#include <vector>
#include <csignal>
#include <math.h>

#define MAX_SIZE 524
#define HEADER_SIZE 12
#define MAX_SEQ_ACK_NO 102400
#define RETRANS_TIMEOUT 0.5 
#define INITIAL_CWND 512
#define MAX_CWND 51200
#define RWND 51200
#define INITIAL_SSTHRESH 10000
#define INITIAL_SEQ_NO 4321

void runError(int code);
void endProgram();
void makeConnection();
void makeSocket();
void signalHandler(int signum);
void processHeader(unsigned char*);

//pointer to our file writers for each connection
std::vector<std::ofstream*> connections;

//directory to store files
std::string direc;

int sock;

char* port;

int res;

int buffSize = 512;

//ints for handling headers
uint32_t currSeq;
uint32_t currAck;
uint16_t currID;
//0 is ack, 1 is syn, 2 is fin
bool flags[3];

//address
struct sockaddr_in servaddr;

int main(int argc, char** argv){
    unsigned char test[] = {
        0x00, 0x00, 0x08, 0x52,
        0x00, 0x00, 0x15, 0x32,
        0x10, 0x01, 0x00, 0x02};
    processHeader(test);
    /*char buf[buffSize];
    if (argc != 3)
    {
        runError(1);
    }
    
    //reads port, tries to catch invalid port numbers
    //only catches port numbers that are not numbers
    //TODO find other invalid port numbers?
    try {
        port = argv[1];
    }
    catch (const std::invalid_argument& ia)
    {
        runError(2);
    }
    
    direc = argv[2];
    std::cerr << argv[1] << std::endl << argv[2] << std::endl;
    
    makeSocket();

    makeConnection();

    signal(SIGQUIT, signalHandler);
    while (1) {
        char buf[1024];
        memset(buf, '\0', 1024);
        struct sockaddr addr;
        socklen_t addr_len = sizeof(struct sockaddr);

        ssize_t length = recvfrom(sock, buf, 1024, 0, &addr, &addr_len);
        std::cerr << "DATA reveived " << length << " bytes from : " << inet_ntoa(((struct sockaddr_in*) & addr)->sin_addr) << std::endl;
        (*connections[0]).write(buf, 1024);

        processHeader(buf);

        length = sendto(sock, "ACK", strlen("ACK"), MSG_CONFIRM, &addr, addr_len);
        std::cout << length << " bytes ACK sent" << std::endl;
    }

    endProgram();*/

	//check arguments
	if(argc != 2){
		std::cerr <<"ERROR: Usage: "<< argv[0] << " <PORT> "<< std::endl;
		exit(1);
	}
	char* p;
	long portNumber = strtol(argv[1], &p, 10);
	if (*p) {
		std::cerr <<"ERROR: Incorrect port number: "<< argv[1] << std::endl;
		exit(1);
	}
	else {
		if (portNumber < 0 || portNumber > 65536){
			std::cerr <<"ERROR: Incorrect port number: "<< argv[1] << std::endl;
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
		std::cerr << "ERROR: getaddrinfo" << std::endl;
		exit(1);
	}
	if (bind(serverSockFd, myAddrInfo->ai_addr, myAddrInfo->ai_addrlen) == -1){
		std::cerr << "ERROR: bind" << std::endl;
		exit(1);
	}
	while (1){
		char buf[1024]; //extra space to be safe
		struct sockaddr addr;
		socklen_t addr_len = sizeof(struct sockaddr);
		
		ssize_t length = recvfrom(serverSockFd, buf, 1024, 0, &addr, &addr_len);

		std::cerr << "DATA reveived " << length << " bytes from : "<< 
		inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr) << std::endl;
		
		length = sendto(serverSockFd, "ACK", strlen("ACK"), MSG_CONFIRM, &addr, 
		addr_len);
		std::cout << length << " bytes ACK sent" << std::endl; 
	}
	exit(0);
}

void signalHandler(int signum)
{
    endProgram();
    exit(0);
}

//0-3 chars are seq number
//4-7 chars are ack number
//8-9 chars are connection ID
//10 is unused
//last 3 bits of 11 are ack, syn, and fin
void processHeader(unsigned char *buf)
{
    currSeq = (buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);

    currAck = (buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7]);

    currID = (buf[8] << 8 | buf[9]);

    flags[0] = (buf[11] >> 2) & 1;
    flags[1] = (buf[11] >> 1) & 1;
    flags[2] = (buf[11] >> 0) & 1;

    std::cerr << currSeq << std::endl;
    std::cerr << currAck << std::endl;
    std::cerr << currID << std::endl;
    std::cerr << flags[0] << flags[1] << flags[2] << std::endl;
}

void makeSocket()
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct addrinfo hints;
    memset(&hints, '\0', sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* myAddrInfo;
    int ret;
    if ((ret = getaddrinfo(NULL, port, &hints, &myAddrInfo)) != 0) {
        std::cerr << gai_strerror(ret) << std::endl;
        runError(6);
    }

    if (bind(sock, myAddrInfo->ai_addr, myAddrInfo->ai_addrlen) == -1) {
        runError(4);
    }
}

//put anything we need to close here
//should always be run before any exits
void endProgram()
{
    for (long unsigned int x = 0; x < connections.size(); x++)
    {
        (*connections[x]).close();
    }
    close(sock);
}

//run this everytime we make a connection to a client
//This works with a premade directory and a given direc with no leading /
//I dont know how its going to be tested, may need to be changed
void makeConnection()
{
    connections.push_back(new std::ofstream(direc + "/" + 
        std::to_string(connections.size() + 1) + ".file"));
    std::cerr << "connection made" << std::endl;
}

//simple error function
void runError(int code)
{
    switch (code)
    {
    case 1:
        std::cerr << "ERROR: Incorrect Number of Arguments!" << std::endl;
        break;
    case 2:
        std::cerr << "ERROR: Incorrect Port Number!" << std::endl;
        break;
    case 3:
        std::cerr << "ERROR: Socket Creation Failed!" << std::endl;
        break;
    case 4:
        std::cerr << "ERROR: Bind Creation Failed!" << std::endl;
        break;

    case 5:
        std::cerr << "ERROR: Socket Reading Failed!" << std::endl;
        break;

    case 6:
        std::cerr << "ERROR: Incorrect Address!" << std::endl;
        break;
    }
    endProgram();
    exit(code);
}