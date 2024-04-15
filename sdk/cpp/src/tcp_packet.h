
#ifndef NETWORK_EXP4_SDK_TCP_PACKET_H
#define NETWORK_EXP4_SDK_TCP_PACKET_H

#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <vector>
#include <arpa/inet.h>
#include <netinet/tcp.h>


class TCPPacket {
public:
    TCPPacket(
        const std::string& src_ip,
        uint16_t src_port,
        const std::string& dts_ip,
        uint16_t dst_port,
        uint32_t seq_num,
        uint32_t ack_num,
        uint8_t flags = 0,
        const std::vector<uint8_t>& cont_data = std::vector<uint8_t>()
    );

    std::vector<uint8_t> build();

private:
    std::string src_ip;
    uint16_t src_port;
    std::string dts_ip;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t flags;
    std::vector<uint8_t> cont_data;

    uint16_t chksum(const std::vector<uint8_t> packet);
    std::vector<uint8_t> serialize_tcphdr(const tcphdr& header);
    std::vector<uint8_t> parseIPAddress(const std::string& ip_string);

};

#endif //NETWORK_EXP4_SDK_TCP_PACKET_H
