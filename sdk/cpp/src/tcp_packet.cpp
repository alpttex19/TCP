/*
* tcp_packet.cpp
* 用于构建TCP数据包，包含TCP报文构建，序列化，校验和计算等功能
*/

#include "tcp_packet.h"

/*
* 构造函数，用于构建一个TCP数据包
*/
TCPPacket::TCPPacket(
    const std::string& src_ip,
    uint16_t src_port,
    const std::string& dts_ip,
    uint16_t dst_port,
    uint32_t seq_num,
    uint32_t ack_num,
    uint8_t flags,
    const std::vector<uint8_t>& cont_data
    ) : src_ip(src_ip),
        src_port(src_port),
        dts_ip(dts_ip),
        dst_port(dst_port),
        seq_num(seq_num),
        ack_num(ack_num),
        flags(flags),
        cont_data(cont_data)
    {}

/*
* 用于将TCP数据包序列化为字节流（大端序）
*/
std::vector<uint8_t> TCPPacket::build() {
    struct tcphdr tcp_header;
    tcp_header.th_sport = htons(src_port);    // Source Port
    tcp_header.th_dport = htons(dst_port);    // Destination Port
    tcp_header.th_seq = htonl(seq_num);        // Sequence Number
    tcp_header.th_ack = htonl(ack_num);        // Acknowledgement Number
    tcp_header.th_off = 5;                     // Data Offset
    tcp_header.th_x2 = 0;
    tcp_header.th_flags = flags;               // Flags
    tcp_header.th_win = htons(65530);          // Window
    tcp_header.th_sum = 0;                     // Checksum (initial value)
    tcp_header.th_urp = 0;                     // Urgent pointer

    std::vector<uint8_t> packet = TCPPacket::serialize_tcphdr(tcp_header);

    if ( cont_data.size() != 0){
        packet.insert(packet.end(), cont_data.begin(), cont_data.end());
    }
    // 伪首部，为了计算校验和
    std::string pseudo_hdr; 
    pseudo_hdr.append(src_ip);
    pseudo_hdr.append(".");
    pseudo_hdr.append(dts_ip);
    pseudo_hdr.append(".0.6");  // Protocol ID and TCP Length
    // 将ip地址转换为字节流
    uint16_t packet_size = ((uint16_t)packet.size());
    std::vector<uint8_t> pseudo = parseIPAddress(pseudo_hdr);

    pseudo.push_back(packet_size >> 8);  // 高字节
    pseudo.push_back(packet_size & 0xFF);  // 低字节        
    std::vector<uint8_t> pseudo_copy = pseudo;
    pseudo_copy.insert(pseudo_copy.end(), packet.begin(), packet.end());
    uint16_t checksum = chksum(pseudo_copy);
    tcp_header.th_sum = htons(checksum);
    memcpy(&packet[16], &tcp_header.th_sum, sizeof(tcp_header.th_sum));
    return packet;
}

// 用于计算校验和，包括TCP头部和伪首部
uint16_t TCPPacket::chksum(const std::vector<uint8_t> packet) {
    uint32_t sum = 0;
    size_t i = 0;
    size_t packetSize = packet.size();

    while (i < packetSize - 1) {
        sum += (packet[i] << 8) + packet[i + 1];
        i += 2;
    }

    if (i < packetSize)
        sum += packet[i] << 8;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum;
}

// 用于序列化TCP头部
std::vector<uint8_t> TCPPacket::serialize_tcphdr(const tcphdr& header) {
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

// 用于将字符串形式的IP地址转换为字节流
std::vector<uint8_t> TCPPacket::parseIPAddress(const std::string& ip_string) {
    std::vector<uint8_t> result;
    std::stringstream ss(ip_string);
    std::string token;
    
    while (std::getline(ss, token, '.')) {
        try {
            uint8_t value = static_cast<uint8_t>(std::stoi(token));
            result.push_back(value);
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid token: " << token << std::endl;
        }
    }
    
    return result;
}