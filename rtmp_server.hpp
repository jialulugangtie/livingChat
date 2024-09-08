#pragma once
#include <memory>

#include "tcp_server.hpp"
#include "rtmp_conn.hpp"
#include "thread_pool.hpp"

namespace mms {
    class RtmpServer : public TcpServer<RtmpConn> {
    public:
        RtmpServer(ThreadWorker* w) :TcpServer(w) {
        }

        bool start(uint16_t port = 1935) {
            if (0 == start_listen(port)) {
                return true;
            }
            return false;
        }

        void stop() {
            stop_listen();
        }
    private:
        void on_tcp_socket_open(std::shared_ptr<TcpSocket> socket) override;
        void on_tcp_socket_close(std::shared_ptr<TcpSocket> socket) override;
    };
};