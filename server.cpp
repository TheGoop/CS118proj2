#include <string>
#include <cstring>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void runError(int code);
void endProgram();
void makeConnection();
void makeSocket();

//pointer to our file writers for each connection
std::vector<std::ofstream*> connections;

//directory to store files
std::string direc;

//socket
int sock;
int port;

//address
struct sockaddr_in servaddr;

//TODO process wrong port numbers
int main(int argc, char** argv)
{
    if (argc != 3)
    {
        runError(1);
    }
    port = std::stoi(argv[1]);
    direc = argv[2];
    std::cerr << argv[1] << std::endl << argv[2] << std::endl;
    makeSocket();
    endProgram();
    return 0;
}

void makeSocket()
{
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        runError(3);
    }
    if ((bind(sock, (const struct sockaddr*)&servaddr, sizeof(servaddr))) == -1)
    {
        runError(4);
    }
}

//put anything we need to close here
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
    }
    endProgram();
    exit(code);
}
