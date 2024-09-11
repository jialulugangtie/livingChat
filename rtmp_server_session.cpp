#include <boost/algorithm/string.hpp>

#include "spdlog/spdlog.h"
#include "rtmp_server_session.hpp"
#include "socket_interface.hpp"

#include "session.hpp"

using namespace mms;
RtmpServerSession::RtmpServerSession(std::shared_ptr<SocketInterface> conn) : StreamSession(conn->get_worker()),
conn_(conn), handshake_(conn), chunk_protocol_(conn),
recv_coroutine_exited_(get_worker()->get_io_context()) {
    set_session_type("rtmp");
}

RtmpServerSession::~RtmpServerSession() {
    spdlog::debug("destroy RtmpServerSession");
}

void RtmpServerSession::service() {
    start_recv_coroutine();
}

void RtmpServerSession::start_recv_coroutine() {
    auto self(shared_from_this());
    recv_coroutine_running_ = true;
    boost::asio::co_spawn(conn_->get_worker()->get_io_context(), [this, self]()->boost::asio::awaitable<void> {
        // 启动握手
        if (!co_await handshake_.do_server_handshake()) {
            co_return;
        }
        // chunk处理
        spdlog::info("RtmpServerSession do_server_handshake ok");
        //循环接受chunk层收到的rtmp消息
        int32_t ret = co_await chunk_protocol_.cycle_recv_rtmp_message(std::bind(&RtmpServerSession::on_recv_rtmp_message, this, std::placeholders::_1));
        co_return;
        co_return;
        }, [this, self](std::exception_ptr exp) {
            (void)exp;
            close();
            recv_coroutine_exited_.close();
            recv_coroutine_running_ = false;
            spdlog::info("RtmpServerSession recv coroutine exited");
            });
}

boost::asio::awaitable<void> RtmpServerSession::stop_recv_coroutine() {
    if (recv_coroutine_running_) {
        if (recv_coroutine_exited_.is_open()) {
            boost::system::error_code ec;
            co_await recv_coroutine_exited_.async_receive(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }
    }
    co_return;
}

boost::asio::awaitable<bool> RtmpServerSession::on_recv_rtmp_message(std::shared_ptr<RtmpMessage>& rtmp_msg){
    spdlog::info("got a rtmp message");
    if (chunk_protocol_.is_protocol_control_message(rtmp_msg)) {
        if (!co_await chunk_protocol_.handle_protocol_control_message(rtmp_msg)) {
            co_return -18;
        }
    }
    co_return true;
}


void RtmpServerSession::close() {
    if (closed_.test_and_set(std::memory_order_acquire)) {
        return;
    }

    auto self(this->shared_from_this());
    boost::asio::co_spawn(get_worker()->get_io_context(), [this, self]()->boost::asio::awaitable<void> {
        boost::system::error_code ec;
        if (conn_) {
            conn_->close();
        }

        co_await stop_recv_coroutine();
        if (conn_) {
            conn_.reset();
        }
        spdlog::info("RtmpServerSession closed");
        co_return;
        }, boost::asio::detached);
}