/**
 * 这是等待你完成的代码。正常情况下，本文件是你唯一需要改动的文件。
 * 你可以任意地改动此文件，改动的范围当然不限于已有的五个函数里。（只要已有函数的签名别改，要是签名改了main里面就调用不到了）
 * 在开始写代码之前，请先仔细阅读此文件和api文件。这个文件里的五个函数是等你去完成的，而api里的函数是供你调用的。
 * 提示：TCP是有状态的协议，因此你大概率，会需要一个什么样的数据结构来记录和维护所有连接的状态
 */
#include "outgoing.h"
#include <netinet/in.h>
#include <cstdint>
#include <unordered_map>
#include <map>
#include "tcp_packet.h"

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

/*
* 状态机的结构体
*/
struct State {
    int connect_state; // 状态0,1,2,3,4,5
    int client_isn;
    int server_ack;  // 最新server发来报文的ack
    int current_ack;  // 当前想发送报文的ack
    int seq_tmp;  // seq缓存，用于验证重传
    ConnectionIdentifier tmp_conn;
    std::map<std::string, int> tmp_save;  // 或者使用其他合适的类型
};

std::unordered_map<std::string, State> state_machine;

/**
 * 当有应用想要发起一个新的连接时，会调用此函数。想要连接的对象在conn里提供了。
 * 你应该向想要连接的对象发送SYN报文，执行三次握手的逻辑。
 * 当连接建立好后，你需要调用app_connected函数，通知应用层连接已经被建立好了。
 * @param conn 连接对象
 */
void app_connect(ConnectionIdentifier &conn) {
    // TODO 请实现此函数
    std::cout << "app_connect" << conn << std::endl;
    std::string key = hash_conn(conn);
    if (state_machine.find(key) == state_machine.end()) { // state machine中不存在当前连接，于是新建当前连接记录
        state_machine[key] = State{0, 0, 0, 0, 0, conn, {}};
    }

    if (state_machine[key].connect_state == 0) {
        std::cout << "发送第一次SYN" << std::endl;

        state_machine[key].connect_state = 1;

        int flags = 0x02;
        state_machine[key].client_isn = 0;
        int seq_num = state_machine[key].client_isn;
        int ack_num = state_machine[key].connect_state;
        std::vector<uint8_t> packet = TCPPacket(     
                            conn.src.ip, 
                            conn.src.port, 
                            conn.dst.ip, 
                            conn.dst.port, 
                            seq_num, 
                            ack_num, 
                            flags
                        ).build();
        //std::vector<uint8_t> packet_bytes(packet.begin(), packet.end());
        tcp_tx(conn, packet);
    }
    else if (state_machine[key].connect_state == 1){
        std::cout << "发送握手ack" << std::endl;

        state_machine[key].connect_state = 2;
        int flags = 0x10;

        int seq_num = state_machine[key].client_isn + 1;
        state_machine[key].current_ack += 1;
        int ack_num = state_machine[key].current_ack;
        std::vector<uint8_t> packet = TCPPacket(     
                            conn.src.ip, 
                            conn.src.port, 
                            conn.dst.ip, 
                            conn.dst.port, 
                            seq_num, 
                            ack_num, 
                            flags
                        ).build();
        //std::vector<uint8_t> packet_bytes(packet.begin(), packet.end());
        tcp_tx(conn, packet);
        app_connected(conn);
    }
    std::cout << "app_connect" << conn << "is over" << std::endl;
}

/**
 * 当应用层想要在一个已经建立好的连接上发送数据时，会调用此函数。
 * @param conn 连接对象
 * @param bytes 数据内容，是字节数组
 */
void app_send(ConnectionIdentifier &conn, std::vector<uint8_t> &bytes) {
    // TODO 请实现此函数
    std::cout << "app_send" << conn << bytes.data() << std::endl;
    std::string key = hash_conn(conn);
    std::cout << "在已建立的连接上发送数据" << std::endl;
    int flags;
    if (bytes.empty()) {
        int flags = 0x10;
    }
    else {
        int flags = 0x18;
    }
    int seq_num = state_machine[key].server_ack;
    int ack_num = state_machine[key].current_ack;
    std::vector<uint8_t> packet = TCPPacket(     
                            conn.src.ip, 
                            conn.src.port, 
                            conn.dst.ip, 
                            conn.dst.port, 
                            seq_num, 
                            ack_num, 
                            flags
                        ).build();
    // std::vector<uint8_t> packet_bytes(packet.begin(), packet.end());
    tcp_tx(conn, packet);
    state_machine[key].server_ack += packet.size();
}

/**
 * 当应用层想要半关闭连接(FIN)时，会调用此函数。
 * @param conn 连接对象
 */
void app_fin(ConnectionIdentifier &conn) {
    // TODO 请实现此函数
    std::cout << "app_fin" << conn << std::endl;
}

/**
 * 当应用层想要重置连接(RES)时，会调用此函数
 * @param conn 连接对象
 */
void app_rst(ConnectionIdentifier &conn) {
    // TODO 请实现此函数
    std::cout << "app_rst" << conn << std::endl;
}

/**
 * 当收到TCP报文时，会调用此函数。
 * 正常情况下，你会对TCP报文，根据报文内容和连接的当前状态加以处理，然后调用0个~多个api文件中的函数
 * @param conn 连接对象
 * @param bytes TCP报文内容，是字节数组。（含TCP报头，不含IP报头）
 */
void tcp_rx(ConnectionIdentifier &conn, std::vector<uint8_t> &bytes) {
    // TODO 请实现此函数
    std::cout << "tcp_rx" << conn << bytes.data() << std::endl;
    std::string key = hash_conn(conn);
    state_machine[key].tmp_conn = conn;
    // 将报文解析为scr_port, dst_port, seq_num, ack_num, mixed, window_size, checksum, urgent_pointer
    std::string packet(bytes.begin(), bytes.end());
    // 按位解析
    std::cout << "解析报文" << packet << std::endl;
    std::string src_port = packet.substr(0, 8);


}

/**
 * 这个函数会每至少100ms调用一次，以保证控制权可以定期的回到你实现的函数中，而不是一直阻塞在main文件里面。
 * 它可以被用来在不开启多线程的情况下实现超时重传等功能，详见主仓库的README.md
 */
void tick() {
    // TODO 可实现此函数，也可不实现
}

// Function to serialize TcpHeader into bytes
// std::vector<uint8_t> serializeTcpHeader(const TcpHeader& header, std::vector<uint8_t>& bytes) {
//     // Serialize source port (big-endian)
//     bytes.push_back(header.source_port >> 8);
//     bytes.push_back(header.source_port & 0xFF);

//     // Serialize destination port (big-endian)
//     bytes.push_back(header.dest_port >> 8);
//     bytes.push_back(header.dest_port & 0xFF);

//     // Serialize sequence number (big-endian)
//     bytes.push_back(header.seq_num >> 24);
//     bytes.push_back((header.seq_num >> 16) & 0xFF);
//     bytes.push_back((header.seq_num >> 8) & 0xFF);
//     bytes.push_back(header.seq_num & 0xFF);

//     // Serialize acknowledgment number (big-endian)
//     bytes.push_back(header.ack_num >> 24);
//     bytes.push_back((header.ack_num >> 16) & 0xFF);
//     bytes.push_back((header.ack_num >> 8) & 0xFF);
//     bytes.push_back(header.ack_num & 0xFF);

//     // Serialize data offset, reserved, and flags (in one byte)
//     bytes.push_back(header.data_offset);
//     bytes.push_back(header.reserved);
//     bytes.push_back(header.flags);

//     // Serialize window size (big-endian)
//     bytes.push_back(header.window_size >> 8);
//     bytes.push_back(header.window_size & 0xFF);

//     // Serialize checksum (big-endian)
//     bytes.push_back(header.checksum >> 8);
//     bytes.push_back(header.checksum & 0xFF);

//     // Serialize urgent pointer (big-endian)
//     bytes.push_back(header.urgent_pointer >> 8);
//     bytes.push_back(header.urgent_pointer & 0xFF);

//     return bytes;
// }

/*
* 将connectionidentifier 哈希
*/
std::string hash_conn(const ConnectionIdentifier& conn) {
    return (conn.src.ip + std::to_string(conn.src.port) + conn.dst.ip + std::to_string(conn.dst.port));
};
