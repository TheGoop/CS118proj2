/*
Starter code from Tianyuan Yu's Week 7 slides
*/

/*
Server: "RECV" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"]
Server: "SEND" <Sequence Number> <Acknowledgement Number> <Connection ID> ["ACK"] ["SYN"] ["FIN"] ["DUP"]
*/

#include <string>
#include <cstring>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <csignal>
#include <math.h>

#include "utils.h"
#include "constants.h"

void runError(int code);
void endProgram();
void makeConnection(std::string direc);
void makeSocket(char *port);
void signalHandler(int signum);

// pointer to our file writers for each connection
std::vector<std::ofstream *> connections;

int sock;
// address
struct sockaddr_in servaddr;

int main(int argc, char **argv)
{
    char *port;

    // directory to store files
    std::string direc;

    // ints for handling headers
    uint32_t currSeq = INITIAL_SERVER_SEQ;
    uint32_t currAck = 0;
    uint16_t currID = 0;
    uint16_t totalConnections = 0;

    // index 0 is ack, 1 is syn, 2 is fin
    bool flags[3];

    // char test[] = {
    //     0x00, 0x00, 0x08, 0x52,
    //     0x00, 0x00, 0x15, 0x32,
    //     0x10, 0x01, 0x00, 0x02};
    // // Expected Output:
    // // 2130
    // // 5426
    // // 4097
    // // 010
    // processHeader(test, currSeq, currAck, currID, flags);

    if (argc != 3)
    {
        runError(1);
    }

    // reads port, tries to catch invalid port numbers
    // only catches port numbers that are not numbers
    // TODO find other invalid port numbers?
    try
    {
        port = argv[1];
    }
    catch (const std::invalid_argument &ia)
    {
        runError(2);
    }

    direc = argv[2];
    std::cerr << argv[1] << std::endl
              << argv[2] << std::endl;

    makeSocket(port);

    signal(SIGQUIT, signalHandler);

    while (1)
    {
        unsigned char recieved_payload[MAX_PAYLOAD_SIZE];
        unsigned char recieved_msg[MAX_SIZE];
        memset(recieved_msg, '\0', MAX_SIZE);
        struct sockaddr addr;
        socklen_t addr_len = sizeof(struct sockaddr);

        // recieve packet from Client
        ssize_t length = recvfrom(sock, recieved_msg, MAX_SIZE, 0, &addr, &addr_len);
        std::cerr << "DATA received " << length << " bytes from: " << inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr) << std::endl;

        processHeader(recieved_msg, currSeq, currAck, currID, flags);
        printServerMessage("RECV", currSeq, currAck, currID, flags);

        // if this is a SYN packet from client (Aka new client/new connection)
        if (flags[1] && !flags[0])
        {
            // printf("SYN RECIEVED\n");
            // Allocate a file writer for this new client
            makeConnection(direc);
            unsigned char msg[HEADER_SIZE] = "";
            currAck = incrementSeq(currSeq, 1);
            currSeq = INITIAL_SERVER_SEQ;
            totalConnections = incrementConnections(totalConnections, 1);
            currID = totalConnections;
            createHeader(msg, currSeq, currAck, currID, SYN_ACK, flags);

            length = sendto(sock, msg, HEADER_SIZE, MSG_CONFIRM, &addr, addr_len);
            printServerMessage("SEND", currSeq, currAck, currID, flags);

            std::cout << length << " bytes sent" << std::endl;
        }

        // if its a normal packet - NO SYN/ACK/SYN-ACK/FIN/FIN-ACK
        else if (!flags[0] && !flags[1] && !flags[2])
        {
            // printf("DATA PAYLOAD PACKET RECIEVED\n");

            // check to see if this connection ID is valid and not already terminated
            if (currID < connections.size() && connections[currID - 1] != NULL)
            {
                // write to connections[currID - 1]
                processPayload(recieved_msg, recieved_payload);
                *connections[currID - 1] << recieved_payload;
            }
            else
            {
                // received packet is dropped (e.g., unknown connection ID):
                printServerMessage("DROP", currSeq, currAck, currID, flags);
            }
        }
        memset(flags, '\0', NUM_FLAGS);

        // unsigned char head[HEADER_SIZE];
        // createHeader(head, INITIAL_SERVER_SEQ, currSeq + 1, currID, SYN_ACK);
        // processHeader(recieved_msg, currSeq, currAck, currID, flags);
        // length = sendto(sock, head, HEADER_SIZE, MSG_CONFIRM, &addr, addr_len);
        // std::cout << length << " bytes ACK sent" << std::endl;
    }

    endProgram();
    return 0;
}

void signalHandler(int signum)
{
    endProgram();
    exit(0);
}

void makeSocket(char *port)
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct addrinfo hints;
    memset(&hints, '\0', sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *myAddrInfo;
    int ret;
    if ((ret = getaddrinfo(NULL, port, &hints, &myAddrInfo)) != 0)
    {
        std::cerr << gai_strerror(ret) << std::endl;
        runError(6);
    }

    if (bind(sock, myAddrInfo->ai_addr, myAddrInfo->ai_addrlen) == -1)
    {
        runError(4);
    }
}

// put anything we need to close here
// should always be run before any exits
void endProgram()
{
    for (size_t x = 0; x < connections.size(); x++)
    {
        (*connections[x]).close();
    }
    close(sock);
}

/*
    connections is mapped this way
    connections[i] contains the File Writer for the (i-1)th client
    first client's file writer -> connections[0]
    second client's file writer -> connections[1]
*/
void makeConnection(std::string direc)
{
    connections.push_back(new std::ofstream(direc + "/" +
                                                std::to_string(connections.size() + 1) + ".file",
                                            std::ios::app));
}

// simple error function
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