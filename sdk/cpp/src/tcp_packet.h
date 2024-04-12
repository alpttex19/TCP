
#ifndef NETWORK_EXP4_SDK_TCP_PACKET_H
#define NETWORK_EXP4_SDK_TCP_PACKET_H

#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <arpa/inet.h>
#include <netinet/tcp.h>

// struct tcphdr {
//     uint16_t th_sport;  // Source Port
//     uint16_t th_dport;  // Destination Port
//     uint32_t th_seq;    // Sequence Number
//     uint32_t th_ack;    // Acknowledgement Number
//     uint8_t th_off;     // Data Offset
//     uint8_t th_flags;   // Flags
//     uint16_t th_win;    // Window
//     uint16_t th_sum;    // Checksum
//     uint16_t th_urp;    // Urgent pointer
// };


std::vector<uint8_t> serialize_tcphdr(const tcphdr& header) {
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

std::vector<uint8_t> parseIPAddress(const std::string& ip_string) {
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
    ) : src_ip(src_ip),
        src_port(src_port),
        dts_ip(dts_ip),
        dst_port(dst_port),
        seq_num(seq_num),
        ack_num(ack_num),
        flags(flags),
        cont_data(cont_data)
    {}

    std::vector<uint8_t> build() {
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

        std::vector<uint8_t> packet = serialize_tcphdr(tcp_header);

        if ( cont_data.size() != 0){
            packet.insert(packet.end(), cont_data.begin(), cont_data.end());
        }

        std::string pseudo_hdr; //伪首部，为了计算校验和
        pseudo_hdr.append(src_ip);
        pseudo_hdr.append(".");
        pseudo_hdr.append(dts_ip);
        pseudo_hdr.append(".0.6");  // Protocol ID and TCP Length
        // packet size不对
        // pseudo_hdr.append(std::to_string(packet_size));
        // std::cout << pseudo_hdr << "\n";
        uint16_t packet_size = ((uint16_t)packet.size());
        std::vector<uint8_t> pseudo = parseIPAddress(pseudo_hdr);
        pseudo.push_back(packet_size >> 8);  // 高字节
        pseudo.push_back(packet_size & 0xFF);  // 低字节        

        // std::cout << "---------------pseudo---------------" <<std::endl;
        // for (size_t i = 0; i < pseudo.size(); ++i) {
        //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pseudo[i]) << " ";
        // }
        // std::cout << std::endl;
        // std::cout << "---------------packet---------------" <<std::endl;
        // for (size_t i = 0; i < packet.size(); ++i) {
        //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(packet[i]) << " ";
        // }
        // std::cout << std::endl;
        std::vector<uint8_t> pseudo_copy = pseudo;
        pseudo_copy.insert(pseudo_copy.end(), packet.begin(), packet.end());
        // std::cout << "---------------pseudo_copy---------------" <<std::endl;
        // for (size_t i = 0; i < pseudo_copy.size(); ++i) {
        //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pseudo_copy[i]) << " ";
        // }
        // std::cout << std::endl;

        uint16_t checksum = chksum(pseudo_copy);
        tcp_header.th_sum = htons(checksum);
        memcpy(&packet[16], &tcp_header.th_sum, sizeof(tcp_header.th_sum));

        // std::cout << "---------------packet---------------" <<std::endl;
        // for (size_t i = 0; i < packet.size(); ++i) {
        //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(packet[i]) << " ";
        // }
        // std::cout << std::endl;
        return packet;
    }

private:
    std::string src_ip;
    uint16_t src_port;
    std::string dts_ip;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t flags;
    std::vector<uint8_t> cont_data;

    uint16_t chksum(const std::vector<uint8_t> packet) {
        uint32_t sum = 0;
        size_t i = 0;
        size_t packetSize = packet.size();

        // Sum up all 16-bit words
        while (i < packetSize - 1) {
            sum += (packet[i] << 8) + packet[i + 1];
            i += 2;
        }

        // If the packet size is odd, add the last byte
        if (i < packetSize)
            sum += packet[i] << 8;

        // Fold 32-bit sum to 16 bits
        while (sum >> 16)
            sum = (sum & 0xFFFF) + (sum >> 16);

        // Take one's complement of the sum
        return ~sum;
    }
};

#endif //NETWORK_EXP4_SDK_TCP_PACKET_H
