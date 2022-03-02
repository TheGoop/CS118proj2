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

//ints for handling headers
uint32_t currSeq;
uint32_t currAck;
uint16_t currID;
//0 is ack, 1 is syn, 2 is fin
bool flags[3];

void processHeader(const char* buf)
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

int main(int argc, char** argv) {
    // check if received three command arguments
    if (argc != 4) {
        std::cerr << "ERROR: Usage: " << argv[0] <<
            " <HOSTNAME-OR-IP> <PORT> <FILENAME>" << std::endl;
        exit(1);
    }

    // set the hints for getaddrinfo()
    struct addrinfo hints;
    struct addrinfo* result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;  // UDP socket
    hints.ai_family = AF_INET;  // IPv4 
    // get server address info using hints
    // argv[1]: HOSTNAME-OR-IP
    // argv[2]: PORT#
    int ret;
    if ((ret = getaddrinfo(argv[1], argv[2], &hints, &result)) != 0) {
        std::cerr << "ERROR: " << ret << std::endl;
        exit(1);
    }

    sockaddr* serverSockAddr = result->ai_addr;
    socklen_t serverSockAddrLength = result->ai_addrlen;
    // create a UDP socket1
    int serverSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    // open file to transfer from client to server
    // argv[3]: FILENAME
    int fileToTransferFd = open(argv[3], O_RDONLY);
    if (fileToTransferFd == -1) {
        std::cerr << "ERROR: open()" << std::endl;
        exit(1);
    }
    struct stat fdStat;
    fstat(fileToTransferFd, &fdStat);
    uint8_t fileBuffer[12];
    size_t bytesRead = read(fileToTransferFd, fileBuffer, 12);
    std::cout << bytesRead << " bytes read" << std::endl;

    const char test[] = {
        0x00, 0x00, 0x30, 0x39,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02};

    sendto(serverSockFd, test, 12, MSG_CONFIRM, serverSockAddr,
        serverSockAddrLength);

    std::cout << "DATA sent" << std::endl;
    struct sockaddr addr;
    socklen_t addr_len = sizeof(struct sockaddr);
    memset(fileBuffer, 0, sizeof(fileBuffer));
    ssize_t length = recvfrom(serverSockFd, fileBuffer, 12, 0, &addr,
        &addr_len);

    processHeader((char*)fileBuffer);
    std::string str((char*)fileBuffer);
    std::cerr << "ACK reveived " << length << " bytes: " << std::endl
        << str << std::endl;
    close(fileToTransferFd);
    exit(0);
}