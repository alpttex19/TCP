#include "tcp_packet.h"



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
    std::vector<uint8_t> pseudo_copy = pseudo;
    pseudo_copy.insert(pseudo_copy.end(), packet.begin(), packet.end());
    uint16_t checksum = chksum(pseudo_copy);
    tcp_header.th_sum = htons(checksum);
    memcpy(&packet[16], &tcp_header.th_sum, sizeof(tcp_header.th_sum));
    return packet;
}


uint16_t TCPPacket::chksum(const std::vector<uint8_t> packet) {
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


std::vector<uint8_t> TCPPacket::serialize_tcphdr(const tcphdr& header) {
    std::vector<uint8_t> buffer(sizeof(header));
    std::memcpy(buffer.data(), &header, sizeof(header));
    return buffer;
}

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