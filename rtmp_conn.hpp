#pragma once
#include <memory>
#include <unordered_map>
#include "thread_worker.hpp"
#include "tcp_socket.hpp"
// #include "server/rtmp/rtmp_server_session.hpp"

namespace mms {
    class RtmpConn;
    class RtmpServerSession;

    class RtmpConn : public TcpSocket {
        friend class RtmpServerSession;
    public:
        RtmpConn(TcpSocketHandler* handler, boost::asio::ip::tcp::socket sock, ThreadWorker* worker) :TcpSocket(handler, std::move(sock), worker) {
        }

        RtmpConn(TcpSocketHandler* handler, ThreadWorker* worker) :TcpSocket(handler, worker) {
        }

        virtual ~RtmpConn();
    };
};