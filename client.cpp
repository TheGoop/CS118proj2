/*
Starter code from Tianyuan Yu's Week 7 slides
*/

/*
Client: "SEND" <Sequence Number> <Acknowledgement Number> <Connection ID> <CWND> <SS-THRESH> ["ACK"] ["SYN"] ["FIN"] ["DUP"]
Client: "RECV" <Sequence Number> <Acknowledgement Number> <Connection ID> <CWND> <SS-THRESH> ["ACK"] ["SYN"] ["FIN"]
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
#include <signal.h>

#include "constants.h"
#include "utils.h"

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGRTMIN

using namespace std;

void handshake(int sockfd, struct sockaddr *addr, socklen_t addr_len,
			   uint32_t &server_seq_no, uint32_t &server_ack_no, uint16_t &connection_id,
			   uint32_t &client_seq_no, uint32_t &client_ack_no, bool *flags)
{
	memset(flags, '\0', NUM_FLAGS);
	// send syn
	unsigned char buf[HEADER_SIZE];
	createHeader(buf, client_seq_no, client_ack_no, connection_id, SYN, flags);

	int length = sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

	cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	// receive syn-ack
	memset(buf, '\0', HEADER_SIZE);
	memset(flags, '\0', NUM_FLAGS);

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

	cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	// send ack is completed after handshake
}

static void timerend(union sigval val)
{
	cerr << "Timer ended" << endl;
	exit(0);
}

void teardown(int sockfd, struct sockaddr *addr, socklen_t addr_len,
			  uint32_t &server_seq_no, uint32_t &server_ack_no, uint16_t &connection_id,
			  uint32_t &client_seq_no, bool *flags)
{
	memset(flags, '\0', NUM_FLAGS);

	unsigned char buf[HEADER_SIZE];
	createHeader(buf, client_seq_no, 0, connection_id, FIN, flags);

	int length = sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

	cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", client_seq_no, 0, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	// receive syn-ack
	memset(buf, '\0', HEADER_SIZE);
	memset(flags, '\0', NUM_FLAGS);

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

	cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	/* Create the timer */
	// 3 elements: ID, timeout value, callback
	union sigval arg;
	arg.sival_int = 54321;
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = timerend;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value = arg;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
	{
		cerr << "Timer create error" << endl;
		exit(1);
	}
	/* Start the timer */
	its.it_value.tv_sec = 2;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 2;
	its.it_interval.tv_nsec = 0;

	cerr << "Timer start" << endl;
	if (timer_settime(timerid, 0, &its, NULL) == -1)
	{
		cerr << "Timer set error" << endl;
		exit(1);
	}

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags, false);

	cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	memset(flags, '\0', NUM_FLAGS);
	memset(buf, '\0', HEADER_SIZE);

	createHeader(buf, server_ack_no, incrementSeq(server_seq_no, 1), connection_id, ACK, flags);

	length = sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

	cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", server_ack_no, incrementSeq(server_seq_no, 1), connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	// TODO: Handle retransmissions for FIN/ACK
	// Code exits at timer handler
	while(1);
}

int main(int argc, char **argv)
{
	// Check if number of args is correct
	if (argc != 4)
	{
		cerr << "ERROR: Usage: ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>" << endl;
		exit(1);
	}

	char *hostname = argv[1];
	// Check for invalid port number
	char *p;
	long portNumber = strtol(argv[2], &p, 10); // convert string to number, base 10
	if (*p)
	{
		cerr << "ERROR: Incorrect port number: " << argv[2] << endl;
		exit(1);
	}
	else
	{
		if (portNumber < 0 || portNumber > 65536)
		{
			cerr << "ERROR: Incorrect port number: " << argv[2] << endl;
			exit(1);
		}
	}
	char *fileName = argv[3];
	// set the hints for getaddrinfo()
	struct addrinfo hints;
	struct addrinfo *result;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM; // UDP socket
	hints.ai_family = AF_INET;		// IPv4
	// get server address info using hints
	int ret;
	if ((ret = getaddrinfo(hostname, argv[2], &hints, &result)) != 0)
	{
		cerr << "ERROR: Incorrect hostname or IP" << ret << endl;
		exit(1);
	}

	sockaddr *addr = result->ai_addr;
	socklen_t addr_len = result->ai_addrlen;
	// create a UDP socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	uint32_t server_seq_no = INITIAL_SERVER_SEQ;
	uint32_t server_ack_no = 0;
	uint16_t connection_id = 0;

	uint32_t client_seq_no = INITIAL_CLIENT_SEQ;
	uint32_t client_ack_no = 0;

	bool flags[NUM_FLAGS]; // ASF

	// Also updates the seq_no, ack_no, conn_id
	handshake(sockfd, addr, addr_len, server_seq_no, server_ack_no, connection_id, client_seq_no, client_ack_no, flags);

	unsigned char buf[MAX_PACKET_SIZE];
	memset(flags, '\0', NUM_FLAGS);

	client_seq_no = server_ack_no;
	client_ack_no = server_seq_no + 1;

	createHeader(buf, client_seq_no, client_ack_no, connection_id, ACK, flags);
	// open file to transfer from client to server
	int filefd = open(fileName, O_RDONLY);
	if (filefd == -1)
	{
		cerr << "ERROR: unable to open file" << endl;
		exit(1);
	}
	struct stat fdStat;
	fstat(filefd, &fdStat);
	size_t bytesRead = 0, totalBytes = fdStat.st_size, counter = 0;

	if (totalBytes >= MAX_PAYLOAD_SIZE)
	{
		bytesRead = read(filefd, buf + HEADER_SIZE, MAX_PAYLOAD_SIZE);
	}
	else
	{
		bytesRead = read(filefd, buf + HEADER_SIZE, totalBytes);
	}

	totalBytes -= bytesRead;
	counter += bytesRead;

	int length = sendto(sockfd, buf, HEADER_SIZE + bytesRead, MSG_CONFIRM, addr, addr_len);

	cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	memset(buf, '\0', HEADER_SIZE);
	memset(flags, '\0', NUM_FLAGS);

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

	cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

	while (totalBytes > 0)
	{
		memset(buf, '\0', HEADER_SIZE);
		memset(flags, '\0', NUM_FLAGS);

		client_seq_no = incrementSeq(server_ack_no, 0);
		client_ack_no = incrementAck(server_seq_no, 1);

		createHeader(buf, client_seq_no, client_ack_no, connection_id, 0, flags);

		if (totalBytes >= MAX_PAYLOAD_SIZE)
		{
			bytesRead = read(filefd, buf + HEADER_SIZE, MAX_PAYLOAD_SIZE);
		}
		else
		{
			bytesRead = read(filefd, buf + HEADER_SIZE, totalBytes);
		}
		counter += bytesRead;
		totalBytes -= bytesRead;
		length = sendto(sockfd, buf, HEADER_SIZE + bytesRead, MSG_CONFIRM, addr, addr_len);
		printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);

		memset(buf, '\0', HEADER_SIZE);
		memset(flags, '\0', NUM_FLAGS);
		length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

		processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

		printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, INITIAL_CWND, INITIAL_SSTHRESH, flags);
		usleep(50000);
	}
	cerr << counter << " bytes read from file" << endl;

	if (fdStat.st_size != server_ack_no - INITIAL_CLIENT_SEQ - 1)
	{
		cerr << "Server has not successfully received all bytes" << endl;
		exit(1);
	}
	cerr << "Sending FIN..." << endl;
	teardown(sockfd, addr, addr_len, server_seq_no, server_ack_no, connection_id, server_ack_no, flags);

	// close(filefd);
	// exit(0);
}
