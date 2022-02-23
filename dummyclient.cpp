#include <string>
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct sockaddr_in addr;
int sock, res;
char buf[] = { 'h', 'e', 'l', 'l', 'o', '\n' };
int main()
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) std::cerr << "shit" << std::endl;

    //Hardcoded port number 8080
    addr.sin_port = htons(8080);

    addr.sin_family = AF_INET;

    addr.sin_addr.s_addr = (((((127 << 8) | 0) << 8) | 0) << 8) | 1;;

    connect(sock, (const struct sockaddr*)&addr, sizeof(addr));
    res = send(sock, (char*)&buf, sizeof(buf), 0);
    if (res == -1) std::cerr << "fuck" << std::endl;
    std::cerr << "finished" << std::endl;
    close(sock);
}