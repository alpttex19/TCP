/**
 * 这是等待你完成的代码。正常情况下，本文件是你唯一需要改动的文件。
 * 你可以任意地改动此文件，改动的范围当然不限于已有的五个函数里。（只要已有函数的签名别改，要是签名改了main里面就调用不到了）
 * 在开始写代码之前，请先仔细阅读此文件和api文件。这个文件里的五个函数是等你去完成的，而api里的函数是供你调用的。
 * 提示：TCP是有状态的协议，因此你大概率，会需要一个什么样的数据结构来记录和维护所有连接的状态
 */
#include "outgoing.h"


std::unordered_map<std::string, State> state_machine;

/**
 * 当有应用想要发起一个新的连接时，会调用此函数。想要连接的对象在conn里提供了。
 * 你应该向想要连接的对象发送SYN报文，执行三次握手的逻辑。
 * 当连接建立好后，你需要调用app_connected函数，通知应用层连接已经被建立好了。
 * @param conn 连接对象
 */
void app_connect(ConnectionIdentifier &conn) {
    // TODO 请实现此函数
    // std::cout << "app_connect" << conn << std::endl;
    std::string key = hash_conn(conn);
    if (state_machine.find(key) == state_machine.end()) { // state machine中不存在当前连接，于是新建当前连接记录
        state_machine[key] = State{0, 0, 0, 0, 0, conn, {}};
    }

    if (state_machine[key].connect_state == 0) {
        std::cout << "发送第一次SYN" << std::endl;
        state_machine[key].connect_state = 1;
        uint8_t flags = 0x02;
        state_machine[key].client_isn = 0x1111;
        uint32_t seq_num = state_machine[key].client_isn;
        uint32_t ack_num = state_machine[key].connect_state;
        send_packet(conn, flags, seq_num, ack_num);
    }
    else if (state_machine[key].connect_state == 1){
        std::cout << "发送第三次握手ack" << std::endl;
        state_machine[key].connect_state = 2;
        uint8_t flags = 0x10;
        uint32_t seq_num = state_machine[key].client_isn + 1;
        state_machine[key].current_ack += 1;
        uint32_t ack_num = state_machine[key].current_ack;
        send_packet(conn, flags, seq_num, ack_num);
        app_connected(conn);
    }
}

/**
 * 当应用层想要在一个已经建立好的连接上发送数据时，会调用此函数。
 * @param conn 连接对象
 * @param bytes 数据内容，是字节数组
 */
void app_send(ConnectionIdentifier &conn, std::vector<uint8_t> &bytes) {
    // TODO 请实现此函数
    // std::cout << "app_send" << conn << std::endl;
    std::string key = hash_conn(conn);
    std::cout << "在已建立的连接上发送数据" << std::endl;
    uint8_t flags = 0x10;
    uint32_t seq_num = state_machine[key].server_ack;
    uint32_t ack_num = state_machine[key].current_ack;
    if (bytes.size() == 0) {
        // std::cout<< "要发送的数据大小为0\n";
        flags = 0x10;
        send_packet(conn, flags, seq_num, ack_num);
    }
    else {
        // std::cout<< "要发送的数据大小为"<< bytes.size() << "\n";
        flags = 0x18;
        send_packet(conn, flags, seq_num, ack_num, bytes);
    }
    state_machine[key].server_ack += bytes.size();
}

/**
 * 当应用层想要半关闭连接(FIN)时，会调用此函数。
 * @param conn 连接对象
 */
void app_fin(ConnectionIdentifier &conn) {
    // TODO 请实现此函数
    // std::cout << "app_fin" << conn << std::endl;
    std::string key = hash_conn(conn);
    std::cout << "发送半关闭连接(FIN)" << std::endl;
    if (state_machine[key].connect_state == 2){
        state_machine[key].connect_state = 3;
    }
    uint8_t flags = 0x11;
    uint32_t seq_num = state_machine[key].server_ack;
    uint32_t ack_num = state_machine[key].current_ack;
    send_packet(conn, flags, seq_num, ack_num);
}

/**
 * 当应用层想要重置连接(RES)时，会调用此函数
 * @param conn 连接对象
 */
void app_rst(ConnectionIdentifier &conn) {
    // TODO 请实现此函数
    // std::cout << "app_rst" << conn << std::endl;
    std::string key = hash_conn(conn);
    std::cout << "发送重置连接请求(RES)" << std::endl;
    uint8_t flags = 0x14;
    uint32_t seq_num = state_machine[key].server_ack;
    uint32_t ack_num = state_machine[key].current_ack;
    send_packet(conn, flags, seq_num, ack_num);
    release_connection(conn);
    // 重置TCP状态机
    state_machine[key].connect_state = 0;
    state_machine[key].client_isn = 0;
    state_machine[key].server_ack = 0;
    state_machine[key].current_ack = 0;
    state_machine[key].seq_tmp = 0;
    state_machine[key].tmp_save = {};
}

/**
 * 当收到TCP报文时，会调用此函数。
 * 正常情况下，你会对TCP报文，根据报文内容和连接的当前状态加以处理，然后调用0个~多个api文件中的函数
 * @param conn 连接对象
 * @param bytes TCP报文内容，是字节数组。（含TCP报头，不含IP报头）
 */
void tcp_rx(ConnectionIdentifier &conn, std::vector<uint8_t> &bytes) {
    // TODO 请实现此函数
    // std::cout << "tcp_rx" << conn << std::endl;
    std::string key = hash_conn(conn);
    state_machine[key].tmp_conn = conn;
    // 将报文解析为scr_port, dst_port, seq_num, ack_num, mixed, window_size, checksum, urgent_pointer
    uint16_t src_port =       ntohs(bytes[0] | bytes[1]<<8);
    uint16_t dst_port =       ntohs(bytes[2] | bytes[3]<<8);
    uint32_t seq_num =        ntohl(bytes[4] | bytes[5]<<8 | bytes[6]<<16 | bytes[7]<<24);
    uint32_t ack_num =        ntohl(bytes[8] | bytes[9]<<8 | bytes[10]<<16 | bytes[11]<<24);
    uint16_t mixed =           ntohs(bytes[12] | bytes[13]<<8);
    uint16_t window_size =    ntohs(bytes[14] | bytes[15]<<8);
    uint16_t checksum =       ntohs(bytes[16] | bytes[17]<<8);
    uint16_t urgent_pointer = ntohs(bytes[18] | bytes[19]<<8);
    // 解析mixed为fin, syn, rst, ack
    int Offset = (mixed >> 12);
    int fin = (mixed & 0x01);
    int syn = (mixed & 0x02) >> 1;
    int rst = (mixed & 0x04) >> 2;
    int ack = (mixed & 0x10) >> 4;
    std::vector<uint8_t> real_data;
    real_data.insert(real_data.end(),bytes.begin() + Offset * 4, bytes.end());

    if (state_machine[key].current_ack == 0) {
        state_machine[key].current_ack = seq_num;
    }
    
    if (state_machine[key].current_ack == seq_num) {
        state_machine[key].server_ack = ack_num;
        state_machine[key].current_ack = seq_num + real_data.size();
        if (syn == 1 && state_machine[key].connect_state == 1) {
            std::cout<<"收到SYN=1的报文，进行第三次握手"<<std::endl;
            app_connect(conn);
        }
        // 对面发来信息，我传给应用层
        else if (state_machine[key].connect_state == 2 && fin == 0){
            std::cout<<"收到报文，发送给应用层"<<std::endl;
            app_recv(conn, real_data);
            if (real_data.size() > 0){
                send_ack(conn);
            }
        }
        // 对面主动发来fin
        else if (state_machine[key].connect_state == 2 && fin == 1){
            std::cout<<"收到FIN=1的报文，发送给上层应用"<<std::endl;
            state_machine[key].connect_state = 5;
            state_machine[key].current_ack += 1;
            app_peer_fin(conn);
            send_ack(conn);
        }
        // 收到第二次挥手：收到对面回应的ack
        else if (state_machine[key].connect_state == 3 && ack == 1){
            std::cout<<"收到ACK报文，服务器准备关闭连接"<<std::endl;
            state_machine[key].connect_state = 4;
        }
        // 收到第三次挥手，进行第四次挥手：收到对面回应的fin，回一个ack
        else if (state_machine[key].connect_state == 4 && fin == 1){
            std::cout<<"收到FIN=1的报文，发送给上层应用"<<std::endl;
            state_machine[key].connect_state = 5;
            state_machine[key].current_ack += 1;
            app_peer_fin(conn);
            send_ack(conn);
            std::cout << "释放连接" << std::endl;
            release_connection(conn);
            state_machine[key].connect_state = 0;
            state_machine[key].client_isn = 0;
            state_machine[key].server_ack = 0;
            state_machine[key].current_ack = 0;
            state_machine[key].seq_tmp = 0;
            state_machine[key].tmp_save = {};
        }
        else if (state_machine[key].connect_state == 5 && ack == 1){
            std::cout<<"收到ACK报文，连接关闭"<<std::endl;
        }
        else {
            std::cout<<"未知状态"<<std::endl;
            return ;
        }

        if (rst == 1){
            std::cout<<"收到RST报文，发送给上层应用"<<std::endl;
            app_peer_rst(conn);
            std::cout << "释放连接" << std::endl;
            state_machine[key].connect_state = 0;
            state_machine[key].client_isn = 0;
            state_machine[key].server_ack = 0;
            state_machine[key].current_ack = 0;
            state_machine[key].seq_tmp = 0;
            state_machine[key].tmp_save = {};
        }
    }
    else {
        std::cout<<"当前的ack和seq不匹配，将报文存入tmp_save"<<std::endl;
        state_machine[key].tmp_save[seq_num] = bytes;
    }
}

/**
 * 这个函数会每至少100ms调用一次，以保证控制权可以定期的回到你实现的函数中，而不是一直阻塞在main文件里面。
 * 它可以被用来在不开启多线程的情况下实现超时重传等功能，详见主仓库的README.md
 */
void tick() {
    // TODO 可实现此函数，也可不实现
    // std::cout<< "tick" << std::endl;
    // 使用迭代器遍历
    for (auto it = state_machine.begin(); it != state_machine.end(); ++it) {
        auto key = it->first;
        if (state_machine[key].connect_state == 2){
            if (state_machine[key].seq_tmp == state_machine[key].current_ack 
                    &&  state_machine[key].seq_tmp != 0){ 
                send_ack(state_machine[key].tmp_conn);
            }
            else {
                state_machine[key].seq_tmp = state_machine[key].current_ack;
            }               
        }
    }
}

// 发送ack
void send_ack(ConnectionIdentifier& conn){
    std::string key = hash_conn(conn);
    uint8_t flags = 0x10;
    uint32_t seq_num = state_machine[key].server_ack;
    uint32_t ack_num = state_machine[key].current_ack;
    send_packet(conn, flags, seq_num, ack_num); 
}

// 发送TCP数据包
void send_packet(ConnectionIdentifier& conn, uint8_t flags, uint32_t seq_num, uint32_t ack_num, const std::vector<uint8_t> &cont_data){
    std::string key = hash_conn(conn);
    std::vector<uint8_t> packet = TCPPacket(     
                            conn.src.ip, 
                            conn.src.port, 
                            conn.dst.ip, 
                            conn.dst.port, 
                            seq_num, 
                            ack_num, 
                            flags,
                            cont_data
                        ).build();
    tcp_tx(conn, packet);
}

/*
* 将connectionidentifier 哈希
*/
std::string hash_conn(const ConnectionIdentifier& conn) {
    return (conn.src.ip + std::to_string(conn.src.port) + conn.dst.ip + std::to_string(conn.dst.port));
};
