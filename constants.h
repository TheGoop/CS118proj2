#ifndef CONSTANTS_H
#define CONSTANTS_H

const int MAX_PAYLOAD = 512;
const int HEADER_SIZE = 12;
const int MAX_SIZE = 524;
const int MAX_SEQ_NO = 102400;
const int RETRANS_TIMEOUT = 0.5;
const int INITIAL_CWND = 512;
const int MAX_CWND = 51200;
const int RWND = 51200;
const int INITIAL_SSTHRESH = 10000;

const uint32_t INITIAL_SERVER_SEQ = 4321;
const uint32_t INITIAL_CLIENT_SEQ = 12345;

#endif