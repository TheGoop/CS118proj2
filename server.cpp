#include <string>
#include <cstring>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <csignal>

void runError(int code);
void endProgram();
void makeConnection();
void makeSocket();
void signalHandler(int signum);

//pointer to our file writers for each connection
std::vector<std::ofstream*> connections;

//directory to store files
std::string direc;

int sock;

int port;

int res;

int buffSize = 512;

//address
struct sockaddr_in servaddr;

int main(int argc, char** argv)
{
    char buf[buffSize];
    if (argc != 3)
    {
        runError(1);
    }
    
    //reads port, tries to catch invalid port numbers
    //only catches port numbers that are not numbers
    //TODO find other invalid port numbers?
    try {
        port = std::stoi(argv[1]);
    }
    catch (const std::invalid_argument& ia)
    {
        runError(2);
    }
    
    direc = argv[2];
    std::cerr << argv[1] << std::endl << argv[2] << std::endl;
    
    makeSocket();

    signal(SIGQUIT, signalHandler);
    while (1) {
        char buf[1024]; //extra space to be safe
        struct sockaddr addr;
        socklen_t addr_len = sizeof(struct sockaddr);

        ssize_t length = recvfrom(serverSockFd, buf, 1024, 0, &addr, &addr_len);
        // std::string str(buf);
        std::cerr << "DATA reveived " << length << " bytes from : " << inet_ntoa(((struct sockaddr_in*) & addr)->sin_addr) << std::endl;

        length = sendto(serverSockFd, "ACK", strlen("ACK"), MSG_CONFIRM, &addr, addr_len);
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

void makeSocket()
{
    struct addrinfo hints;
    memset(&hints, '\0', sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* myAddrInfo;
    int ret;
    if ((ret = getaddrinfo(NULL, direc.c_str(), &hints, &myAddrInfo)) != 0) {
        runError(3);
    }

    if (bind(sock, myAddrInfo->ai_addr, myAddrInfo->ai_addrlen) == -1) {
        runError(4);
    }
}

//put anything we need to close here
//should always be run before any exits
void endProgram()
{
    for (int x = 0; x < connections.size(); x++)
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
    }
    endProgram();
    exit(code);
}
