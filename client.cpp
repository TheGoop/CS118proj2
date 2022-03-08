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
#include <set>
#include <vector>

#include "constants.h"
#include "utils.h"

using namespace std;

int filefd;
int ssthresh = INITIAL_SSTHRESH;
int cwnd = INITIAL_CWND;
uint16_t connection_id;

struct itimerspec its2 = {{0, 0}, {0, 0}};

timer_t rttid;
union sigval argrtt;
struct sigevent sevrtt;
struct itimerspec itsrtt;

std::set<int> awaited_acks;

struct reTransObject
{
	int sockfd;
	sockaddr *addr;
	socklen_t addr_len;
	unsigned char buf[MAX_PACKET_SIZE];
	int arraySize;
	uint32_t seq;
	uint32_t ackId;
};

std::vector<reTransObject> sentPackets;

// This is called after 10 seconds of nothing being received
void outoftime(union sigval val)
{
	// cerr << "10 seconds exceeded" << endl;
	// if (filefd != NULL)
	close(filefd);
	exit(1);
}

void retransmit(union sigval val)
{
	// std::cerr << "Retransmit packet from latest ACK'd byte" << std::endl;

	ssthresh = cwnd / 2;
	cwnd = INITIAL_CWND;

	lseek(filefd, sentPackets[0].seq - 12346, SEEK_SET);

	int length = sendto(sentPackets[0].sockfd, sentPackets[0].buf, sentPackets[0].arraySize, MSG_CONFIRM, sentPackets[0].addr, sentPackets[0].addr_len);
	bool flags[3] = {false, false, false};
	// cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", sentPackets[0].seq, 0, connection_id, cwnd, ssthresh, flags, true);
	// Do stuff to actually retransmit here. Maybe a variable that stores the last ACK'd byte?
}

void handshake(int sockfd, struct sockaddr *addr, socklen_t addr_len,
			   uint32_t &server_seq_no, uint32_t &server_ack_no, uint16_t &connection_id,
			   uint32_t &client_seq_no, uint32_t &client_ack_no, bool *flags)
{
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	/* Create the timer */
	// 3 elements: ID, timeout value, callback
	union sigval arg;
	arg.sival_int = 54322;
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = outoftime;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value = arg;
	if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1)
	{
		cerr << "ERROR: Timer create error" << endl;
		exit(1);
	}
	/* Start the timer */
	its.it_value.tv_sec = 10;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	memset(flags, '\0', NUM_FLAGS);
	// send syn
	unsigned char buf[HEADER_SIZE];
	createHeader(buf, client_seq_no, client_ack_no, connection_id, SYN, flags);

	sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

	// cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, cwnd, ssthresh, flags);

	// receive syn-ack
	memset(buf, '\0', HEADER_SIZE);
	memset(flags, '\0', NUM_FLAGS);

	// Timer that counts to 10 seconds
	if (timer_settime(timerid, 0, &its, NULL) == -1)
	{
		cerr << "ERROR: Timer set error" << endl;
		// close(filefd);
		exit(1);
	}

	// // Reset RTT with new sival
	// struct reTransObject retrans;
	// retrans.sockfd = sockfd;
	// retrans.addr = addr;
	// retrans.addr_len = addr_len;
	// retrans.buf = buf;
	// argrtt.sival_ptr = &retrans;
	// sevrtt.sigev_value = argrtt;
	// if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
	// {
	// 	cerr << "ERROR: Timer create error" << endl;
	// 	exit(1);
	// }

	// // RTT timer that counts to 0.5 seconds. When it reaches that, it calls retransmit and passes in the packet's clientSeq
	if (timer_settime(rttid, 0, &itsrtt, NULL) == -1)
	{
		std::cerr << "ERROR: Timer set error" << std::endl;
		close(filefd);
		exit(1);
	}

	recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

	// disarm the timer
	if (timer_settime(timerid, 0, &its2, NULL) == -1)
	{
		cerr << "ERROR: Timer set error" << endl;
		// close(filefd);
		exit(1);
	}
	// if (timer_settime(rttid, 0, &its2, NULL) == -1)
	// {
	// 	cerr << "ERROR: Timer set error" << endl;
	// 	// close(filefd);
	// 	exit(1);
	// }

	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

	// cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);

	// send ack is completed after handshake
}

static void timerend(union sigval val)
{
	// cerr << "Timer ended" << endl;
	exit(0);
}

void teardown(int sockfd, struct sockaddr *addr, socklen_t addr_len,
			  uint32_t &server_seq_no, uint32_t &server_ack_no, uint16_t &connection_id,
			  uint32_t &client_seq_no, uint32_t &client_ack_no, bool *flags, int cwnd)
{
	memset(flags, '\0', NUM_FLAGS);

	unsigned char buf[HEADER_SIZE];
	createHeader(buf, client_seq_no, 0, connection_id, FIN, flags);

	sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

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
	if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1)
	{
		cerr << "ERROR: Timer create error" << endl;
		exit(1);
	}
	/* Start the timer */
	its.it_value.tv_sec = 2;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	bool isTimerSet = false;

	// cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", client_seq_no, 0, connection_id, cwnd, INITIAL_SSTHRESH, flags);
	while (1)
	{
		// memset(buf, '\0', HEADER_SIZE);
		// memset(flags, '\0', NUM_FLAGS);

		// // Reset RTT with new sival
		// struct reTransObject retrans;
		// retrans.sockfd = sockfd;
		// retrans.addr = addr;
		// retrans.addr_len = addr_len;
		// retrans.buf = buf;
		// argrtt.sival_ptr = &retrans;
		// sevrtt.sigev_value = argrtt;
		// if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
		// {
		// 	cerr << "ERROR: Timer create error" << endl;
		// 	exit(1);
		// }

		recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

		processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

		// cerr << "Total bytes received: " << length << endl;
		// Receive FIN-ACK or ACK
		printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);

		if (server_ack_no)
		{
			client_seq_no = server_ack_no;
		}

		// cerr << "Timer start" << endl;
		if (!isTimerSet)
		{
			if (timer_settime(timerid, 0, &its, NULL) == -1)
			{
				cerr << "ERROR: Timer set error" << endl;
				exit(1);
			}
			isTimerSet = true;
		}
		// memset(buf, '\0', HEADER_SIZE);
		// memset(flags, '\0', NUM_FLAGS);
		// recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

		// processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

		// // cerr << "Total bytes received: " << length << endl;
		// printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);

		if (flags[0] && flags[2])
		{ // FIN-ACK, immediately send ACK to close connection
			// Disarm the RTT since we expect no more ACKs
			// cerr << "Disarm RTT" << endl;
			// if (timer_settime(rttid, 0, &its2, NULL) == -1)
			// {
			// 	std::cerr << "ERROR: RTT disarm error" << std::endl;
			// 	close(filefd);
			// 	exit(1);
			// }

			memset(flags, '\0', NUM_FLAGS);
			memset(buf, '\0', HEADER_SIZE);

			client_ack_no = incrementSeq(server_seq_no, 1);
			createHeader(buf, client_seq_no, client_ack_no, connection_id, ACK, flags);

			// Test RTT on server side waiting for last ACK
			// sleep(3);

			sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

			// cerr << "Total bytes sent: " << length << endl;
			printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);
		}
		else if (flags[0])
		{ // ACK, need to wait to receive FIN, then can send ACK and close connection
			// // Reset RTT with new sival
			// struct reTransObject rts;
			// rts.sockfd = sockfd;
			// rts.addr = addr;
			// rts.addr_len = addr_len;
			// rts.buf = buf;
			// argrtt.sival_ptr = &rts;
			// sevrtt.sigev_value = argrtt;
			// if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
			// {
			// 	cerr << "ERROR: Timer create error" << endl;
			// 	exit(1);
			// }

			// // RTT timer that counts to 0.5 seconds. When it reaches that, it calls retransmit and passes in the packet's clientSeq
			// if (timer_settime(rttid, 0, &itsrtt, NULL) == -1)
			// {
			// 	std::cerr << "ERROR: Timer set error" << std::endl;
			// 	close(filefd);
			// 	exit(1);
			// }

			recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

			// Disarm RTT
			// if (timer_settime(rttid, 0, &its2, NULL) == -1)
			// {
			// 	cerr << "ERROR: Timer set error" << endl;
			// 	// close(filefd);
			// 	exit(1);
			// }

			processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

			// cerr << "Total bytes received: " << length << endl;
			// Receive FIN-ACK or ACK
			printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);
		}
		if (flags[2])
		{
			memset(flags, '\0', NUM_FLAGS);
			memset(buf, '\0', HEADER_SIZE);

			client_ack_no = incrementSeq(server_seq_no, 1);
			createHeader(buf, client_seq_no, client_ack_no, connection_id, ACK, flags);

			sendto(sockfd, buf, HEADER_SIZE, MSG_CONFIRM, addr, addr_len);

			// cerr << "Total bytes sent: " << length << endl;
			printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);
		}
	}
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
	connection_id = 0;

	uint32_t client_seq_no = INITIAL_CLIENT_SEQ;
	uint32_t client_ack_no = 0;

	bool flags[NUM_FLAGS]; // ASF

	/* Create the RTT (0.5 sec) timer */
	// 3 elements: ID, timeout value, callback
	argrtt.sival_int = 8675309;
	sevrtt.sigev_notify = SIGEV_THREAD;
	sevrtt.sigev_notify_function = retransmit;
	sevrtt.sigev_notify_attributes = NULL;
	sevrtt.sigev_value = argrtt;
	if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
	{
		std::cerr << "ERROR: Timer create error" << std::endl;
		exit(1);
	}
	/* Start the timer */
	itsrtt.it_value.tv_sec = 0;
	itsrtt.it_value.tv_nsec = 500000000;
	itsrtt.it_interval.tv_sec = 0;
	itsrtt.it_interval.tv_nsec = 0;

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

	// cerr << "Total bytes sent: " << length << endl;
	printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);

	memset(buf, '\0', HEADER_SIZE);
	memset(flags, '\0', NUM_FLAGS);

	// // Reset RTT with new sival
	// struct reTransObject retrans;
	// retrans.sockfd = sockfd;
	// retrans.addr = addr;
	// retrans.addr_len = addr_len;
	// retrans.buf = buf;
	// argrtt.sival_ptr = &retrans;
	// sevrtt.sigev_value = argrtt;
	// if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
	// {
	// 	cerr << "ERROR: Timer create error" << endl;
	// 	exit(1);
	// }

	// // RTT timer that counts to 0.5 seconds. When it reaches that, it calls retransmit and passes in the packet's clientSeq
	// if (timer_settime(rttid, 0, &itsrtt, NULL) == -1)
	// {
	// 	std::cerr << "ERROR: Timer set error" << std::endl;
	// 	close(filefd);
	// 	exit(1);
	// }

	length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

	// Disarm RTT
	// if (timer_settime(rttid, 0, &its2, NULL) == -1)
	// {
	// 	cerr << "ERROR: Timer set error" << endl;
	// 	// close(filefd);
	// 	exit(1);
	// }

	processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	/* Create the timer */
	// 3 elements: ID, timeout value, callback
	union sigval arg;
	arg.sival_int = 54323;
	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = outoftime;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value = arg;
	if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1)
	{
		cerr << "ERROR: Timer create error" << endl;
		exit(1);
	}
	/* Start the timer */
	its.it_value.tv_sec = 10;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	// cerr << "Total bytes received: " << length << endl;
	printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);
	cwnd += 512;
	while (totalBytes > 0)
	{
		memset(buf, '\0', HEADER_SIZE);
		memset(flags, '\0', NUM_FLAGS);
		if (awaited_acks.size() == 0)
		{
			client_seq_no = incrementSeq(server_ack_no, 0);
			client_ack_no = incrementAck(server_seq_no, 1);
			for (int i = 0; i < cwnd; i += 512)
			{
				if (totalBytes <= 0)
				{
					break;
				}

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
				reTransObject temp;
				temp.seq = client_seq_no;
				uint32_t tempSeq = incrementSeq(client_seq_no, bytesRead);
				awaited_acks.insert(tempSeq);
				temp.ackId = tempSeq;
				temp.sockfd = sockfd;
				temp.addr = addr;
				temp.addr_len = addr_len;
				memset(temp.buf, '\0', HEADER_SIZE + bytesRead);
				memcpy(temp.buf, buf, HEADER_SIZE + bytesRead);
				temp.arraySize = HEADER_SIZE + bytesRead;
				sentPackets.push_back(temp);
				length = sendto(sockfd, buf, HEADER_SIZE + bytesRead, MSG_CONFIRM, addr, addr_len);
				if (flags[0])
				{
					printClientMessage("SEND", client_seq_no, client_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);
				}
				else
				{
					printClientMessage("SEND", client_seq_no, 0, connection_id, cwnd, INITIAL_SSTHRESH, flags);
				}
				
				// Reset RTT with new sival
				// if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
				// {
				// 	cerr << "ERROR: Timer create error" << endl;
				// 	exit(1);
				// }

				// RTT timer that counts to 0.5 seconds. When it reaches that, it calls retransmit and passes in the packet's clientSeq
				if (timer_settime(rttid, 0, &itsrtt, NULL) == -1)
				{
					std::cerr << "ERROR: Timer set error" << std::endl;
					close(filefd);
					exit(1);
				}
				// cerr << "Set timer for " << client_seq_no << endl;

				memset(buf, '\0', HEADER_SIZE);
				memset(flags, '\0', NUM_FLAGS);

				client_seq_no = incrementSeq(client_seq_no, bytesRead);
			}
		}

		// Timer that counts to 10 seconds
		if (timer_settime(timerid, 0, &its, NULL) == -1)
		{
			cerr << "ERROR: Timer set error" << endl;
			close(filefd);
			exit(1);
		}

		// // Reset RTT with new sival
		// struct reTransObject rts;
		// rts.sockfd = sockfd;
		// rts.addr = addr;
		// rts.addr_len = addr_len;
		// retrans.buf = buf;
		// argrtt.sival_ptr = &rts;
		// sevrtt.sigev_value = argrtt;
		// if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
		// {
		// 	cerr << "ERROR: Timer create error" << endl;
		// 	exit(1);
		// }

		// // RTT timer that counts to 0.5 seconds. When it reaches that, it calls retransmit and passes in the packet's clientSeq
		// if (timer_settime(rttid, 0, &itsrtt, NULL) == -1)
		// {
		// 	std::cerr << "ERROR: Timer set error" << std::endl;
		// 	close(filefd);
		// 	exit(1);
		// }

		length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

		// Disarm RTT
		// if (timer_settime(rttid, 0, &its2, NULL) == -1)
		// {
		// 	cerr << "ERROR: Timer set error" << endl;
		// 	// close(filefd);
		// 	exit(1);
		// }

		processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);

		printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);
		if (awaited_acks.find(server_ack_no) != awaited_acks.end())
		{
			vector<set<int>::iterator> less_than;
            set<int>::iterator itr;
            for (itr = awaited_acks.begin(); itr != awaited_acks.end(); itr++)
            {
                if (*itr <= server_ack_no)
                {
                    less_than.push_back(itr);
                }
            }
            // then erase all of those from awaited_acks
            for (int i = 0; i < less_than.size(); i++)
            {
                awaited_acks.erase(less_than[i]);
            }

            for (int x = 0; x < sentPackets.size(); x++)
            {
                if (sentPackets[x].ackId == server_ack_no)
                {
                    // cerr << "Erasing " << sentPackets[x].ackId << endl;
                    sentPackets.erase(sentPackets.begin() + x);
                }
            }

			if (cwnd < ssthresh)
			{
				cwnd = incrementCwnd(cwnd, MAX_PAYLOAD_SIZE);
			}
			else
			{
				cwnd = incrementCwnd(cwnd, (MAX_PAYLOAD_SIZE * MAX_PAYLOAD_SIZE) / cwnd);
			}
		}
		// usleep(50000);
	}
	// cerr << counter << " bytes read from file" << endl;

	// if (fdStat.st_size != server_ack_no - INITIAL_CLIENT_SEQ - 1)
	// {
	// 	cerr << "Server has not successfully received all bytes" << endl;
	// 	exit(1);
	// }
	// cerr << "Sending FIN..." << endl;
	while (awaited_acks.size() != 0)
	{
		// // Reset RTT with new sival
		// struct reTransObject rts;
		// rts.sockfd = sockfd;
		// rts.addr = addr;
		// rts.addr_len = addr_len;
		// retrans.buf = buf;
		// argrtt.sival_ptr = &rts;
		// sevrtt.sigev_value = argrtt;
		// if (timer_create(CLOCK_MONOTONIC, &sevrtt, &rttid) == -1)
		// {
		// 	cerr << "ERROR: Timer create error" << endl;
		// 	exit(1);
		// }

		// // RTT timer that counts to 0.5 seconds. When it reaches that, it calls retransmit and passes in the packet's clientSeq
		// if (timer_settime(rttid, 0, &itsrtt, NULL) == -1)
		// {
		// 	std::cerr << "ERROR: Timer set error" << std::endl;
		// 	close(filefd);
		// 	exit(1);
		// }

		length = recvfrom(sockfd, buf, HEADER_SIZE, 0, addr, &addr_len);

		// // Disarm RTT
		// if (timer_settime(rttid, 0, &its2, NULL) == -1)
		// {
		// 	cerr << "ERROR: Timer set error" << endl;
		// 	// close(filefd);
		// 	exit(1);
		// }

		processHeader(buf, server_seq_no, server_ack_no, connection_id, flags);
		printClientMessage("RECV", server_seq_no, server_ack_no, connection_id, cwnd, INITIAL_SSTHRESH, flags);

		// get list of iterators less than or equal to server_ack_no
        vector<set<int>::iterator> less_than;
        set<int>::iterator itr;
        for (itr = awaited_acks.begin(); itr != awaited_acks.end(); itr++)
        {
            if (*itr <= server_ack_no)
            {
                less_than.push_back(itr);
            }
        }
        // then erase all of those from awaited_acks
        for (int i = 0; i < less_than.size(); i++)
        {
            awaited_acks.erase(less_than[i]);
        }

		for (int x = 0; x < sentPackets.size(); x++)
		{
			if (sentPackets[x].ackId == server_ack_no)
			{
				sentPackets.erase(sentPackets.begin() + x);
			}
		}
		if (cwnd < ssthresh)
		{
			cwnd += 512;
		}
		else
		{
			cwnd += (512 * 512) / cwnd;
		}
	}

	// Disarm RTT timer
	if (timer_settime(rttid, 0, &its2, NULL) == -1)
	{
		cerr << "ERROR: Timer set error" << endl;
		// close(filefd);
		exit(1);
	}

	close(filefd);

	client_seq_no = server_ack_no;
	teardown(sockfd, addr, addr_len, server_seq_no, server_ack_no, connection_id, client_seq_no, client_ack_no, flags, cwnd);

	exit(0);
}