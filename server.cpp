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
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"
#include "constants.h"

void runError(int code);
void endProgram();
void makeConnection(char *direc, u_int16_t currID);
void makeSocket(char *port);
void signalHandler(int signum);

// pointer to our file writers for each connection
std::vector<std::ofstream *> connections;

int sock;
// address
struct sockaddr_in servaddr;

long totalBytes = 0;

int main(int argc, char **argv)
{
    // for out of order writing
    int rollovers = 0;

    char *port;

    // directory to store files
    char *direc;

    // ints for handling headers
    uint32_t currServerSeq = INITIAL_SERVER_SEQ;
    uint32_t currClientSeq = INITIAL_CLIENT_SEQ;
    uint32_t currServerAck = 0;
    uint32_t currClientAck = 0;
    uint16_t currID = 0;
    uint16_t totalConnections = 0;

    // index 0 is ack, 1 is syn, 2 is fin
    bool flags[3];

    if (argc != 3)
    {
        runError(1);
    }

    // reads port, tries to catch invalid port numbers
    // only catches port numbers that are not numbers
    try
    {
        port = argv[1];
    }
    catch (const std::invalid_argument &ia)
    {
        runError(2);
    }

    // Check for invalid port number
    char *p;
    long portNumber = strtol(port, &p, 10); // convert string to number, base 10
    if (*p)
    {
        std::cerr << "ERROR: Incorrect port number: " << port << std::endl;
        exit(1);
    }
    else
    {
        if (portNumber < 0 || portNumber > 65536)
        {
            std::cerr << "ERROR: Incorrect port number: " << port << std::endl;
            exit(1);
        }
    }

    direc = argv[2];
    makeSocket(port);

    signal(SIGQUIT, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    struct sockaddr addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    while (1)
    {
        unsigned char recieved_msg[MAX_PACKET_SIZE];
        memset(recieved_msg, '\0', MAX_PACKET_SIZE);

        // recieve packet from client
        ssize_t bytes_recieved = recvfrom(sock, recieved_msg, MAX_PACKET_SIZE, 0, &addr, &addr_len);
        // std::cerr << "Total bytes received: " << bytes_recieved << std::endl;

        processHeader(recieved_msg, currClientSeq, currClientAck, currID, flags);
        printServerMessage("RECV", currClientSeq, currClientAck, currID, flags);

        // if this is a SYN packet from client (Aka new client/new connection)
        if (flags[1] && !flags[0])
        {
            // printf("SYN RECIEVED\n");
            // Allocate a file writer for this new client
            totalConnections = incrementConnections(totalConnections, 1);
            currID = totalConnections;
            makeConnection(direc, currID);
            unsigned char msg[HEADER_SIZE] = "";
            currServerAck = incrementSeq(currClientSeq, 1);
            createHeader(msg, currServerSeq, currServerAck, currID, SYN_ACK, flags);

            sendto(sock, msg, HEADER_SIZE, MSG_CONFIRM, &addr, addr_len);
            printServerMessage("SEND", currServerSeq, currServerAck, currID, flags);

            // std::cerr << "Total bytes sent: " << bytes_sent << std::endl;
        }

        // if its a normal packet - NO SYN or FIN
        else if (!flags[1] && !flags[2])
        {
            // printf("DATA PAYLOAD PACKET RECIEVED\n");

            // check to see if this connection ID is valid and not already terminated
            if (currID <= connections.size() && connections[currID - 1] != NULL)
            {
                // set the connections pointer to the correct position
                connections[currID - 1]->seekp((currClientSeq + (rollovers * (MAX_SEQ_NO + 1))) - 12346);

                // write to connections[currID - 1]
                for (int i = HEADER_SIZE; i < bytes_recieved; i++)
                {
                    *connections[currID - 1] << recieved_msg[i];
                }
                totalBytes += bytes_recieved - 12;

                // create ACK to send back to client
                unsigned char msg[HEADER_SIZE];
                // currServerSeq = currClientAck;
                currServerSeq = 4322;

                uint32_t temVar = currClientSeq;
                currServerAck = incrementAck(currClientSeq, bytes_recieved - HEADER_SIZE);
                if (temVar > currServerAck)
                    rollovers++;

                createHeader(msg, currServerSeq, currServerAck, currID, ACK, flags);

                sendto(sock, msg, HEADER_SIZE, MSG_CONFIRM, &addr, addr_len);
                printServerMessage("SEND", currServerSeq, currServerAck, currID, flags);
                // std::cerr << "Total bytes sent: " << bytes_sent << std::endl;
            }
            else
            {
                // received packet is dropped (e.g., unknown connection ID):
                printServerMessage("DROP", currClientSeq, currClientAck, currID, flags);
            }
        }

        // FIN stuff
        else if (flags[2])
        {
            while (1)
            {
                memset(flags, '\0', NUM_FLAGS);
                // create FIN_ACK to send back to client
                unsigned char msg[HEADER_SIZE];
                currServerAck = incrementSeq(currClientSeq, 1);
                createHeader(msg, currServerSeq, currServerAck, currID, FIN_ACK, flags);

                sendto(sock, msg, HEADER_SIZE, MSG_CONFIRM, &addr, addr_len);
                printServerMessage("SEND", currServerSeq, currServerAck, currID, flags);
                // std::cerr << "Total bytes sent: " << bytes_sent << std::endl;
                memset(flags, '\0', NUM_FLAGS);
                memset(msg, '\0', HEADER_SIZE);
                recvfrom(sock, msg, HEADER_SIZE, 0, &addr, &addr_len);

                processHeader(msg, currClientSeq, currClientAck, currID, flags);
                printServerMessage("RECV", currClientSeq, currClientAck, currID, flags);
                // If properly receive ACK from client for server FIN, close connection
                if (flags[0])
                {
                    // std::cerr << "Connection " << currID << " closing..." << std::endl;
                    (*connections[currID - 1]).close();
                    currServerAck = 0;
                    currServerSeq = INITIAL_SERVER_SEQ;
                    break;
                }
            }
        }
        memset(flags, '\0', NUM_FLAGS);
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
    std::cout << "Total Bytes: " << totalBytes << std::endl;
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
void makeConnection(char *direc, u_int16_t currID)
{
    char path[128];
    sprintf(path, "%s/%u.file", direc, currID);
    std::ofstream *out = new std::ofstream(path);
    connections.push_back(out);
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
