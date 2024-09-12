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
#include "rtmp_chunk_protocol.hpp"

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
        boost::asio::awaitable<bool> on_recv_rtmp_message(std::shared_ptr<RtmpMessage>& rtmp_msg);
    private:
        std::shared_ptr<SocketInterface> conn_;
        RtmpHandshake handshake_;
        RtmpChunkProtocol chunk_protocol_;
        std::atomic_flag closed_ = ATOMIC_FLAG_INIT;
        bool recv_coroutine_running_ = false;
        boost::asio::experimental::concurrent_channel<void(boost::system::error_code, bool)> recv_coroutine_exited_;

        bool send_coroutine_running_{ false };
        boost::asio::experimental::concurrent_channel<void(boost::system::error_code, bool)> send_coroutine_exited_;
        using SYNC_CHANNEL = boost::asio::experimental::channel<void(boost::system::error_code, bool)>;
        boost::asio::experimental::channel<void(boost::system::error_code, std::vector<std::shared_ptr<RtmpMessage>>, SYNC_CHANNEL*)> rtmp_msgs_channel_;
    };

};