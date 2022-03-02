#ifndef PACKET_H
#define PACKET_H

#include <cstring>

#include "constants.h"

// Flags
// seq_no (32) | ack_no (32) | conn_id (16) | not used (13) | A | S | F
const int F_ACK = 0x4;
const int F_SYN = 0x2;
const int F_FIN = 0x1;

struct Header {
    uint32_t seq_no = 0;
    uint32_t ack_no = 0;
    uint16_t conn_id = 0;
    uint16_t flags = 0; // 0000 0000 0000 0ASF
};

class Packet {
    public:
        Packet() {}
        Packet(Header header, const char* payload, int size) : m_header(header), m_size(size) { memcpy(m_payload, payload, size); }

        uint32_t getSeqNo() const { return m_header.seq_no; } 
        uint32_t getAckNo() const { return m_header.ack_no; } 
        uint16_t getConnectionID() const { return m_header.conn_id; } 
        uint16_t getFlags() const { return m_header.flags; } 
        bool getACK() const { return !!(m_header.flags & F_ACK); } 
        bool getSYN() const { return !!(m_header.flags & F_SYN); } 
        bool getFIN() const { return !!(m_header.flags & F_FIN); } 
        const char* getPayload() const { return m_payload; } 
        int getPayloadSize() const { return m_size; } 
        int getPacketSize() const { return getPayloadSize() + HEADER_SIZE; } 

        void setSeqNo(uint16_t s) { m_header.seq_no = s; }
        void setAckNo(uint16_t a) { m_header.ack_no = a; }
        void setConnectionID(uint16_t c_id) { m_header.conn_id = c_id; }
        void setFlags(uint16_t f) { m_header.flags = f; }
        void setACK() { m_header.flags |= F_ACK; }
        void setSYN() { m_header.flags |= F_SYN; }
        void setFIN() { m_header.flags |= F_FIN; }
        void setPayload(const char *msg, int size) { memcpy(m_payload, msg, size); m_size = size; }
        void setPayloadSize(int size) { m_size = size; }

        bool operator==(const Packet& other) const { return getSeqNo() == other.getSeqNo(); }

    private:
        Header m_header;
        char m_payload[MAX_PAYLOAD];
        int m_size;
};

#endif 