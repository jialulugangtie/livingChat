#include <boost/algorithm/string.hpp>

#include "spdlog/spdlog.h"
#include "rtmp_server_session.hpp"
#include "socket_interface.hpp"

#include "session.hpp"

using namespace mms;
RtmpServerSession::RtmpServerSession(std::shared_ptr<SocketInterface> conn) : StreamSession(conn->get_worker()),
conn_(conn), handshake_(conn),
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
        // ��������
        if (!co_await handshake_.do_server_handshake()) {
            co_return;
        }
        // chunk����
        spdlog::info("RtmpServerSession do_server_handshake ok");
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