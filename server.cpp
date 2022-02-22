#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>

void runError(int code);
void endProgram();
void makeConnection();

//pointer to our file writers for each connection
std::vector<std::ofstream*> connections;

//directory to store files
std::string direc;

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        runError(1);
    }
    direc = argv[2];
    std::cerr << argv[1] << std::endl << argv[2] << std::endl;
    makeConnection();
    endProgram();
    exit(0);
}

//put anything we need to close here
void endProgram()
{
    for (int x = 0; x < connections.size(); x++)
    {
        (*connections[x]).close();
    }
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
        std::cerr << "ERROR: Incorrect Argument Number!" << std::endl;
        break;
    }
    exit(code);
}
