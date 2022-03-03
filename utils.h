#ifndef UTILS_H
#define UTILS_H

#include <iostream>

#include "constants.h"

void createHeader(unsigned char *head, uint32_t seq, uint32_t ack, uint16_t conn_id, uint8_t flag_byte, bool *flags)
{ // seq = 5, ack = 9
    head[0] = (seq >> 24) & 0Xff;
    head[1] = (seq >> 16) & 0Xff;
    head[2] = (seq >> 8) & 0Xff;
    head[3] = (seq >> 0) & 0Xff;
    head[4] = (ack >> 24);
    head[5] = (ack >> 16);
    head[6] = (ack >> 8);
    head[7] = (ack >> 0);
    head[8] = (conn_id >> 8);
    head[9] = (conn_id >> 0);
    head[10] = 0x00;
    head[11] = flag_byte;
    if (flag_byte == SYN)
    {
        flags[1] = 1;
    }
    if (flag_byte == SYN_ACK)
    {
        flags[0] = 1;
        flags[1] = 1;
    }
    if (flag_byte == ACK)
    {
        flags[0] = 1;
    }
    if (flag_byte == FIN)
    {
        flags[2] = 1;
    }
    if (flag_byte == FIN_ACK)
    {
        flags[0] = 1;
        flags[2] = 1;
    }
}

// 0-3 chars are seq number
// 4-7 chars are ack number
// 8-9 chars are connection ID
// 10 is unused
// last 3 bits of 11 are ack, syn, and fin
void processHeader(unsigned char *buf, uint32_t &currSeq, uint32_t &currAck, uint16_t &currID, bool *flags)
{
    currSeq = (buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);

    currAck = (buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7]);

    currID = (buf[8] << 8 | buf[9]);

    flags[0] = (buf[11] >> 2) & 1;
    flags[1] = (buf[11] >> 1) & 1;
    flags[2] = (buf[11] >> 0) & 1;

    // std::cerr << "SeqNo: " << currSeq << std::endl;
    // std::cerr << "AckNo: " << currAck << std::endl;
    // std::cerr << "Conn_ID: " << currID << std::endl;
    // std::cerr << "ASF: " << flags[0] << flags[1] << flags[2] << std::endl;
}

void processPayload(unsigned char *packet, unsigned char *payload)
{
    memcpy(payload, &packet[HEADER_SIZE], MAX_PAYLOAD_SIZE);
}

void printServerMessage(std::string msg, u_int32_t currSeq, u_int32_t currAck, u_int16_t connID, bool *flags)
{
    std::cout << msg << " " << currSeq << " " << currAck << " " << connID;
    if (flags[0])
        std::cout << " ACK";
    if (flags[1])
        std::cout << " SYN";
    if (flags[2])
        std::cout << " FIN";
    std::cout << std::endl;
}

void printClientMessage(std::string msg, u_int32_t currSeq, u_int32_t currAck, u_int16_t connID, int cwnd, int ssthresh, bool *flags)
{
    std::cout << msg << " " << currSeq << " " << currAck << " " << connID << " " << cwnd << " " << ssthresh;
    if (flags[0])
        std::cout << " ACK";
    if (flags[1])
        std::cout << " SYN";
    if (flags[2])
        std::cout << " FIN";
    std::cout << std::endl;
}

uint32_t incrementSeq(uint32_t seq, uint32_t amount)
{
    return (seq + amount) % (MAX_SEQ_NO + 1);
}

uint32_t incrementAck(uint32_t ack, uint32_t amount)
{
    return (ack + amount) % (MAX_SEQ_NO + 1);
}

uint16_t incrementConnections(uint16_t c_id, uint16_t amount)
{
    return c_id + amount;
}
#endif