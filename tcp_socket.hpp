#pragma once
#include <memory>
#include <atomic>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "thread_pool.hpp"
#include "socket_interface.hpp"

namespace mms {
    class TcpSocket;

    class TcpSocketHandler {
    public:
        virtual ~TcpSocketHandler() {}
        virtual void on_tcp_socket_open(std::shared_ptr<TcpSocket> sock) = 0;
        virtual void on_tcp_socket_close(std::shared_ptr<TcpSocket> sock) = 0;
    };

    class TcpSocket : public SocketInterface {
    public:
        TcpSocket(TcpSocketHandler* handler, boost::asio::ip::tcp::socket sock, ThreadWorker* worker);
        TcpSocket(TcpSocketHandler* handler, ThreadWorker* worker);

        virtual ~TcpSocket();
        boost::asio::awaitable<bool> connect(const std::string& ip, uint16_t port, int32_t timeout_ms = 0) override;
        boost::asio::awaitable<bool> send(const uint8_t* data, size_t len, int32_t timeout_ms = 0) override;
        boost::asio::awaitable<bool> send(std::vector<boost::asio::const_buffer>& bufs, int32_t timeout_ms = 0) override;
        boost::asio::awaitable<bool> recv(uint8_t* data, size_t len, int32_t timeout_ms = 0) override;
        boost::asio::awaitable<int32_t> recv_some(uint8_t* data, size_t len, int32_t timeout_ms = 0) override;
        virtual void open();
        void close();
        inline ThreadWorker* get_worker() {
            return worker_;
        }

        bool is_open() {
            return socket_.is_open();
        }
    protected:
        ThreadWorker* worker_ = nullptr;
        boost::asio::ip::tcp::socket socket_;
        TcpSocketHandler* handler_;
        std::atomic_flag closed_ = ATOMIC_FLAG_INIT;
        boost::asio::steady_timer recv_timeout_timer_;
        boost::asio::steady_timer send_timeout_timer_;
        uint64_t id_;
    };
};