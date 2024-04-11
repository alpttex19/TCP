
#ifndef NETWORK_EXP4_SDK_TCP_PACKET_H
#define NETWORK_EXP4_SDK_TCP_PACKET_H

#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <arpa/inet.h>
#include <netinet/tcp.h>


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
        const std::string& data = ""
    ) : src_ip(src_ip),
        src_port(src_port),
        dts_ip(dts_ip),
        dst_port(dst_port),
        seq_num(seq_num),
        ack_num(ack_num),
        flags(flags),
        data(data)
    {}

    std::vector<uint8_t> build() {
        struct tcphdr tcp_header;
        tcp_header.th_sport = htons(src_port);    // Source Port
        tcp_header.th_dport = htons(dst_port);    // Destination Port
        tcp_header.th_seq = htonl(seq_num);        // Sequence Number
        tcp_header.th_ack = htonl(ack_num);        // Acknowledgement Number
        tcp_header.th_off = 5 ;                     // Data Offset
        tcp_header.th_flags = flags;               // Flags
        tcp_header.th_win = htons(65530);          // Window
        tcp_header.th_sum = 0;                     // Checksum (initial value)
        tcp_header.th_urp = 0;                     // Urgent pointer

        std::vector<uint8_t> packet = serialize_tcphdr(tcp_header);
        // for (size_t i = 0; i < packet.size(); ++i) {
        //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(packet[i]) << " ";
        // }
        // std::cout << std::endl;

        if ( data != ""){
            uint8_t* ptr1 = reinterpret_cast<uint8_t*>(&data[0]);
            for (size_t i = 0; i < sizeof(data); ++i) {
                packet.push_back(*ptr1);
                ++ptr1;
            }
        }
        // for (size_t i = 0; i < packet.size(); ++i) {
        //     std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(packet[i]) << " ";
        // }
        // std::cout << std::endl;

        std::string pseudo_hdr; //伪首部，为了计算校验和
        pseudo_hdr.append(src_ip);
        pseudo_hdr.append(".");
        pseudo_hdr.append(dts_ip);
        pseudo_hdr.append(".0.6.0.");  // Protocol ID and TCP Length
        pseudo_hdr.append(std::to_string(packet.size()));
        std::cout << pseudo_hdr << "\n";
        std::vector<uint8_t> pseudo = parseIPAddress(pseudo_hdr);
        for (size_t i = 0; i < pseudo.size(); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(pseudo[i]) << " ";
        }
        std::cout << std::endl;

        std::vector<uint8_t> packet_copy = packet;
        packet_copy.insert(packet_copy.end(), pseudo.begin(), pseudo.end());

        uint16_t checksum = chksum(packet_copy, packet_copy.size());
        tcp_header.th_sum = htons(checksum);
        memcpy(&packet[16], &tcp_header.th_sum, sizeof(tcp_header.th_sum));
        for (size_t i = 0; i < packet.size(); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(packet[i]) << " ";
        }
        std::cout << std::endl;
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
    std::string data;

    uint16_t chksum(const std::vector<uint8_t> packet, int packet_len) {
        // Assuming you want to reinterpret each pair of bytes as uint16_t
        std::vector<uint16_t> packet16;
        for (size_t i = 0; i < packet.size(); i += 2) {
            uint16_t value = (packet[i] << 8) | packet[i + 1];
            packet16.push_back(value);
        }
        uint32_t sum = 0;
        int i = 0;
        while (packet_len > 1) {
            sum += packet16[i++];
            packet_len -= 2;
        }

        // if (packet_len > 0) {
        // }

        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        return static_cast<uint16_t>(~sum);
    }
};

#endif //NETWORK_EXP4_SDK_TCP_PACKET_H
