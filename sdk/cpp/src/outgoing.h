/**
 * 这是SDK内置的代码，本文件是outgoing函数的声明，你应该去outgoing.cpp中对函数进行实现。
 * 函数相关的注释也全在outgoing.cpp里。
 * 此文件通常不用改动。但如果你有确切的理由，也可以自行改动，但请务必确保你清楚自己在做什么！
 * 助教评阅时，会使用你上传的版本。
 */
#ifndef NETWORK_EXP4_SDK_OUTGOING_H
#define NETWORK_EXP4_SDK_OUTGOING_H

#include <netinet/in.h>
#include <cstdint>
#include <unordered_map>
#include <map>
#include "api.h"
#include "tcp_packet.h"

/*
* 状态机的结构体
*/
struct State {
    int connect_state; // 状态0,1,2,3,4,5
    uint32_t client_isn;
    uint32_t server_ack;  // 最新server发来报文的ack
    uint32_t current_ack;  // 当前想发送报文的ack
    uint32_t seq_tmp;  // seq缓存，用于验证重传
    ConnectionIdentifier tmp_conn;
    std::map<uint32_t, std::vector<uint8_t>> tmp_save;  // 或者使用其他合适的类型
};

void app_connect(ConnectionIdentifier &conn);

void app_send(ConnectionIdentifier &conn, std::vector<uint8_t> &bytes);

void app_fin(ConnectionIdentifier &conn);

void app_rst(ConnectionIdentifier &conn);

void tcp_rx(ConnectionIdentifier &conn, std::vector<uint8_t> &bytes);

void tick();

// 将connection对象转换为字符串
std::string hash_conn(const ConnectionIdentifier& conn);
void send_ack(ConnectionIdentifier& conn);
void send_packet(ConnectionIdentifier& conn, 
                              uint8_t flags, 
                              uint32_t seq_num, 
                              uint32_t ack_num, 
                              const std::vector<uint8_t> &cont_data = std::vector<uint8_t>());

#endif //NETWORK_EXP4_SDK_OUTGOING_H

