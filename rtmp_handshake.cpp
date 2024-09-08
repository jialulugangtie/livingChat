#include <array>
#include "spdlog/spdlog.h"

#include "rtmp_handshake.hpp"
using namespace mms;

RtmpHandshake::RtmpHandshake(std::shared_ptr<SocketInterface> conn) :conn_(conn) {
}

RtmpHandshake::~RtmpHandshake() {
}

boost::asio::awaitable<bool> RtmpHandshake::do_server_handshake() {
    std::array<uint8_t, 1537> c0c1;
    if (!(co_await conn_->recv(c0c1.data(), 1537, 4000))) {
        co_return false;
    }

    std::array<uint8_t, 3073> s0s1s2;//1 + 1536 + 1536
    _genS0S1S2(c0c1.data(), s0s1s2.data());
    // send s0, s1, s2
    if (!(co_await conn_->send(s0s1s2.data(), 3073, 4000))) {
        co_return false;
    }

    std::array<uint8_t, 1536> c2;
    if (!(co_await conn_->recv(c2.data(), 1536, 4000))) {
        co_return false;
    }

    co_return true;
}

boost::asio::awaitable<bool> RtmpHandshake::do_client_handshake() {
    uint8_t c0c1[1537];
    c0c1[0] = 0x03;
    *(uint32_t*)(c0c1 + 1) = time(NULL);
    *(uint32_t*)(c0c1 + 5) = 0;
    if (!(co_await conn_->send(c0c1, 1537, 4000))) {
        co_return false;
    }

    uint8_t s0s1s2[3073];
    // send s0, s1, s2
    if (!(co_await conn_->recv(s0s1s2, 3073, 4000))) {
        co_return false;
    }

    uint8_t c2[1536];
    memcpy(c2, s0s1s2, 4);
    memcpy(c2 + 4, c0c1 + 1, 4);
    memcpy(c2 + 8, s0s1s2 + 1537 + 8, 1528);
    if (!(co_await conn_->send(c2, 1536, 4000))) {
        co_return false;
    }

    co_return true;
}

void RtmpHandshake::_genS0S1S2(uint8_t* c0c1, uint8_t* s0s1s2) {
    // s0s1 = c0c1
    memcpy(s0s1s2, c0c1, 1537);
    //s2
    int32_t t = ntohl(*(int32_t*)(c0c1 + 1));
    *(int32_t*)(s0s1s2 + 1537) = htonl(t);  // 复制自C1中的time字段
    *(int32_t*)(s0s1s2 + 1541) = htonl(t);  // 复制自S1中的time字段
    memcpy(s0s1s2 + 1545, c0c1 + 9, 1528);  // 
}