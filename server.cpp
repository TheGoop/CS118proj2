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

#define PACKET_SIZE 524
#define INITIAL_SEQ_NUM 4321
#define PAYLOAD_SIZE 512
#define HEADER_SIZE 12

void runError(int code);
void endProgram();
void makeConnection(std::string direc);
void makeSocket(char *port);
void signalHandler(int signum);
void processHeader(const char *buf, uint32_t &currSeq, uint32_t &currAck, uint16_t &currID, bool *flags);
void createHeader(unsigned char *head, uint32_t seq, uint32_t ack);

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
    uint32_t currSeq;
    uint32_t currAck;
    uint16_t currID;

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
        char buf[PACKET_SIZE];
        memset(buf, '\0', PACKET_SIZE);
        struct sockaddr addr;
        socklen_t addr_len = sizeof(struct sockaddr);

        // recieve packet from Client
        ssize_t length = recvfrom(sock, buf, PACKET_SIZE, 0, &addr, &addr_len);
        std::cerr << "DATA reveived " << length << " bytes from : " << inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr) << std::endl;

        processHeader(buf, currSeq, currAck, currID, flags);

        // if this is a SYN packet from client
        if (flags[1] == 1)
        {
            makeConnection(direc);
            unsigned char msg[HEADER_SIZE] = "";
            createHeader(msg, INITIAL_SEQ_NUM, currSeq + 1);
            // write back to the client
            (*connections[connections.size() - 1]).write((const char *)buf, PACKET_SIZE);
        }

        unsigned char head[HEADER_SIZE];
        createHeader(head, INITIAL_SEQ_NUM, 1);
        processHeader(buf, currSeq, currAck, currID, flags);
        length = sendto(sock, head, HEADER_SIZE, MSG_CONFIRM, &addr, addr_len);
        std::cout << length << " bytes ACK sent" << std::endl;
    }

    endProgram();
    return 0;
}

void signalHandler(int signum)
{
    endProgram();
    exit(0);
}

void createHeader(unsigned char *head, uint32_t seq, uint32_t ack)
{ // seq = 5, ack = 9
    head[0] = (seq >> 24) & 0Xff;
    head[1] = (seq >> 16) & 0Xff;
    head[2] = (seq >> 8) & 0Xff;
    head[3] = (seq >> 0) & 0Xff;
    head[4] = (ack >> 24);
    head[5] = (ack >> 16);
    head[6] = (ack >> 8);
    head[7] = (ack >> 0);
    head[8] = 0x00;
    head[9] = 0x01;
    head[10] = 0x00;
    head[11] = 0x06;
    for (int i = 0; i < 12; i++)
    {
        printf("%d: %u\n", i, head[i]);
    }
}

// 0-3 chars are seq number
// 4-7 chars are ack number
// 8-9 chars are connection ID
// 10 is unused
// last 3 bits of 11 are ack, syn, and fin

void processHeader(const char *buf, uint32_t &currSeq, uint32_t &currAck, uint16_t &currID, bool *flags)
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
    for (int x = 0; x < connections.size(); x++)
    {
        (*connections[x]).close();
    }
    close(sock);
}

// run this everytime we make a connection to a client
// This works with a premade directory and a given direc with no leading /
// I dont know how its going to be tested, may need to be changed
void makeConnection(std::string direc)
{
    connections.push_back(new std::ofstream(direc + "/" +
                                            std::to_string(connections.size() + 1) + ".file"));
    std::cerr << "connection made" << std::endl;
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
