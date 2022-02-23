/* 
From Tianyuan Yu's Week 7 slides
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

int main(int argc, char** argv){
  // check if received three command arguments
  if (argc != 4) {
    std::cerr <<"ERROR: Usage: "<< argv[0] << 
      " <HOSTNAME-OR-IP> <PORT> <FILENAME>"<< std::endl;
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
  if ((ret = getaddrinfo(argv[1], argv[2], &hints, &result)) != 0){
    std::cerr << "ERROR: " << ret << std::endl;
    exit(1);
  }

  sockaddr* serverSockAddr = result->ai_addr;
  socklen_t serverSockAddrLength = result->ai_addrlen;
  // create a UDP socket
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

// udp_retransmission_client.cpp
/* client set timer after each sendto()
 timer will periodically expire
   on each timeout callback, do another sendback()
 when receiving ACK, disarm timer by setting it_value = 0

 set up UDP socket
 send packet
 wait for ACK
    set timer
    if timer expires
      retransmit
      restart timer - interval
    on ACK
      disarm timer
      update congestion/flow control
      continue



// static size_t bytesRead;
// static int serverSockFd;
// static sockaddr* serverSockAddr;
// static socklen_t serverSockAddrLength;
// static void
// handler(union sigval val)
// {
//    std::cout << "retransmission needed " << std::endl;
//    sendto(serverSockFd, val.sival_ptr, bytesRead, MSG_CONFIRM, serverSockAddr, 
// serverSockAddrLength);
//    std::cout << "DATA resent" << std::endl; 
// }
// int main(int argc, char** argv)
// {
// // check if received three command arguments
// if (argc != 4) {
//   std::cerr<<"ERROR: Usage: "<<
//     argv[0] << " <HOSTNAME-OR-IP> <PORT> <FILENAME>"<<std::endl;
//   exit(1);
// }
// // set the hints for getaddrinfo()
// struct addrinfo hints;
// struct addrinfo* result;
// memset(&hints, 0, sizeof(hints));
// hints.ai_socktype = SOCK_DGRAM;  // UDP socket
// hints.ai_family = AF_INET;  // IPv4 
// // get server address info using hints
// // argv[1]: HOSTNAME-OR-IP
// // argv[2]: PORT#
// int ret;
// if ((ret = getaddrinfo(argv[1], argv[2], &hints, &result)) != 0)
// {
//   std::cerr << "ERROR: " << ret << std::endl;
//   exit(1);
// }
// serverSockAddr = result->ai_addr;
// serverSockAddrLength = result->ai_addrlen;
// // create a UDP socket
// serverSockFd = socket(AF_INET, SOCK_DGRAM, 0);
// // open file to transfer from client to server
// // argv[3]: FILENAME
// int fileToTransferFd = open(argv[3], O_RDONLY);
// if (fileToTransferFd == -1) {
//   std::cerr << "ERROR: open()" << std::endl;
//   exit(1);
// }
//     struct stat fdStat;
//     fstat(fileToTransferFd, &fdStat);
//     uint8_t fileBuffer[fdStat.st_size];
//     size_t bytesRead = read(fileToTransferFd, fileBuffer, fdStat.st_size);
//     std::cout << bytesRead << " bytes read" << std::endl;
//     sendto(serverSockFd, fileBuffer, bytesRead, MSG_CONFIRM, serverSockAddr, 
// serverSockAddrLength);
//     std::cout << "DATA sent" << std::endl; 
//     timer_t timerid;
//     struct sigevent sev;
//     struct itimerspec its;
//     /* Create the timer */ 
//     union sigval arg;
//     arg.sival_ptr = fileBuffer;
//     sev.sigev_notify = SIGEV_THREAD;
//     sev.sigev_notify_function = handler;
//     sev.sigev_notify_attributes=NULL;
//     sev.sigev_value = arg;
//     timer_create(CLOCK_MONOTONIC, &sev, &timerid);
//     /* Start the 5 seconds timer */
//     its.it_value.tv_sec = 5;
//     its.it_value.tv_nsec = 0;
//     its.it_interval.tv_sec = 5;
//     its.it_interval.tv_nsec = 0;
//     timer_settime(timerid, 0, &its, NULL);
//     // while (1) {
//     //     usleep(1000);
//     // }
//     struct sockaddr addr;
//     socklen_t addr_len = sizeof(struct sockaddr);
// memset(fileBuffer, 0, sizeof(fileBuffer));
//     ssize_t length = recvfrom(serverSockFd, fileBuffer, fdStat.st_size, 0, &addr, 
// &addr_len);
//     std::string str((char*)fileBuffer);
//     std::cerr << "ACK reveived " << length << " bytes: " << std::endl
//               << str << std::endl;
//     // disarm when receiving ACK
//     its.it_value.tv_sec = 0;
//     its.it_value.tv_nsec = 0;
//     its.it_interval.tv_sec = 0;
//     its.it_interval.tv_nsec = 0;
//     timer_settime(timerid, 0, &its, NULL);
// close(fileToTransferFd);
// exit(0);
// }