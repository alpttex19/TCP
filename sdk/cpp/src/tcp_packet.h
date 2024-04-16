/*
* tcp_packet.h
* 这是一个TCP数据包的类，用于构建TCP数据包，包含TCP报文构建，序列化，校验和计算等功能
* 作者：阿拉帕提
* 
*/

#ifndef NETWORK_EXP4_SDK_TCP_PACKET_H
#define NETWORK_EXP4_SDK_TCP_PACKET_H

#include <iostream>
#include <string>
#include <cstring>
#include <iomanip>
#include <vector>
#include <arpa/inet.h>
#include <netinet/tcp.h>
/*
* 类名：TCPPacket
* 功能：用于构建TCP数据包，包含TCP报文构建，序列化，校验和计算等功能
* 成员变量：
*   src_ip: 源IP地址
*   src_port: 源端口号
*   dts_ip: 目的IP地址
*   dst_port: 目的端口号
*   seq_num: 序列号
*   ack_num: 确认号
*   flags: TCP标志位
*   cont_data: TCP数据包的数据部分
* 成员函数：
*   TCPPacket: 构造函数，用于构建一个TCP数据包
*   build: 用于将TCP数据包序列化为字节流（大端序）
*   chksum: 用于计算校验和
*   serialize_tcphdr: 用于序列化TCP头部
*   parseIPAddress: 用于将字符串形式的IP地址转换为字节流
*/


class TCPPacket {
public:
    /*
    * 默认构造函数，用于构建一个空的TCP数据包
    */
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
    // 用于将TCP数据包序列化为字节流（大端序）
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
    // 用于计算校验和
    uint16_t chksum(const std::vector<uint8_t> packet);
    // 用于序列化TCP头部
    std::vector<uint8_t> serialize_tcphdr(const tcphdr& header);
    // 用于将字符串形式的IP地址转换为字节流
    std::vector<uint8_t> parseIPAddress(const std::string& ip_string);

};

#endif //NETWORK_EXP4_SDK_TCP_PACKET_H
