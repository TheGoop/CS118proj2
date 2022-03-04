#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

const int MAX_PAYLOAD_SIZE = 512;
const int HEADER_SIZE = 12;
const int MAX_PACKET_SIZE = 524;
const int MAX_SEQ_NO = 102400;
const double RETRANS_TIMEOUT = 0.5;
const int INITIAL_CWND = 512;
const int MAX_CWND = 51200;
const int RWND = 51200;
const int INITIAL_SSTHRESH = 10000;

const uint32_t INITIAL_SERVER_SEQ = 4321;
const uint32_t INITIAL_CLIENT_SEQ = 12345;

const uint8_t ACK = 0x4;
const uint8_t SYN = 0x2;
const uint8_t FIN = 0x1;
const uint8_t SYN_ACK = 0x06;
const uint8_t FIN_ACK = 0x05;
const int NUM_FLAGS = 3;

#endif