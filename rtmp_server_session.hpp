#pragma once
#include <memory>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>

#include "stream_session.hpp"
#include "rtmp_handshake.hpp"

namespace mms {
    class SocketInterface;

    class RtmpServerSession : public StreamSession {
    public:
        RtmpServerSession(std::shared_ptr<SocketInterface> conn);
        virtual ~RtmpServerSession();
        void service();
        void close();
    private:
        void start_recv_coroutine();
        boost::asio::awaitable<void> stop_recv_coroutine();
    private:
        std::shared_ptr<SocketInterface> conn_;
        RtmpHandshake handshake_;
        std::atomic_flag closed_ = ATOMIC_FLAG_INIT;
        bool recv_coroutine_running_ = false;
        boost::asio::experimental::concurrent_channel<void(boost::system::error_code, bool)> recv_coroutine_exited_;
    };

};