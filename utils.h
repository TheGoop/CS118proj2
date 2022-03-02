#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <iostream>
#include <string>

#include "constants.h"
#include "packet.h"

Packet makePacket(uint32_t seq, uint32_t ack, uint16_t c_id, int flag, const char *message, int payload_size){
    Header header;
    Packet packet(header, message, payload_size);
    if (flag == 1) packet.setSYN();
    if (flag == 2) packet.setACK();
    if (flag == 3) packet.setFIN();
    if (flag == 4){
        packet.setSYN(); packet.setACK();
    }
    if (flag == 5){
        packet.setFIN(); packet.setACK();
    }
    packet.setSeqNo(seq);
    packet.setAckNo(ack);
    packet.setConnectionID(c_id);
    return packet;
}

bool sendPacket(int sockfd, const char *buf, int length, const struct sockaddr *server_addr, socklen_t addr_len){
    int total = 0, nbytes = length;

    while (total < length) {
        int sent = sendto(sockfd, buf + total, nbytes, MSG_CONFIRM, server_addr, addr_len);
        total += sent;
        nbytes -= sent;
    }
    std::cerr << "Total sent: " << std::__cxx11::to_string(total) << std::endl;
    return true;
}

int recvPacket(int sockfd, Packet& packet, struct sockaddr *server_addr, socklen_t *addr_len){
    int nbytes = recvfrom(sockfd, (char *)&packet, MAX_PAYLOAD, 0, server_addr, addr_len);
    packet.setPayloadSize(nbytes - HEADER_SIZE);
    packet.setSeqNo(packet.getSeqNo());
    packet.setAckNo(packet.getAckNo());
    packet.setConnectionID(packet.getConnectionID());
    packet.setFlags(packet.getFlags());
    return nbytes;
}

#endif