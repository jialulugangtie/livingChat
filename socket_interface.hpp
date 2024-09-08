#pragma once
#include <boost/atomic.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/awaitable.hpp>

#include <memory>
namespace mms {
    class ThreadWorker;
    class Session;
    class SocketInterface : public std::enable_shared_from_this<SocketInterface> {
    public:
        SocketInterface();
        virtual ~SocketInterface();
        virtual boost::asio::awaitable<bool> connect(const std::string& ip, uint16_t port, int32_t timeout_ms = 0) = 0;
        virtual boost::asio::awaitable<bool> send(const uint8_t* data, size_t len, int32_t timeout_ms = 0) = 0;
        virtual boost::asio::awaitable<bool> send(std::vector<boost::asio::const_buffer>& bufs, int32_t timeout_ms = 0) = 0;
        virtual boost::asio::awaitable<bool> recv(uint8_t* data, size_t len, int32_t timeout_ms = 0) = 0;
        virtual boost::asio::awaitable<int32_t> recv_some(uint8_t* data, size_t len, int32_t timeout_ms = 0) = 0;
        virtual void open() = 0;
        virtual void close() = 0;
        virtual ThreadWorker* get_worker() = 0;

        void set_session(std::shared_ptr<Session> session);
        std::shared_ptr<Session> get_session();
        void clear_session();


        int64_t get_in_bytes() {
            return in_bytes_;
        }

        int64_t get_out_bytes() {
            return out_bytes_;
        }
    protected:
        static std::atomic<uint64_t> socket_id_;
    protected:
        std::shared_ptr<Session> session_;

        int64_t in_bytes_ = 0;
        int64_t out_bytes_ = 0;
    };
};