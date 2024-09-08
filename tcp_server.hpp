#pragma once

#include <stdint.h>
#include <string>

#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>

#include "spdlog/spdlog.h"
#include "thread_pool.hpp"
#include "tcp_socket.hpp"

namespace this_coro = boost::asio::this_coro;
using namespace boost::asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;

namespace mms {
    template <typename CONN>
    class TcpServer : public TcpSocketHandler {
    public:
        TcpServer(ThreadWorker* worker) :worker_(worker) {

        }

        virtual ~TcpServer() {

        }
    public:
        virtual int32_t start_listen(uint16_t port, const std::string& addr = "") {
            if (!worker_) {
                return -1;
            }

            running_ = true;
            port_ = port;
            boost::asio::co_spawn(worker_->get_io_context(), ([port, addr, this]()->boost::asio::awaitable<void> {
                boost::asio::ip::tcp::endpoint endpoint;
                endpoint.port(port);
                if (!addr.empty()) {
                    endpoint.address(boost::asio::ip::address::from_string(addr));
                }
                else {
                    endpoint.address(boost::asio::ip::address::from_string("0.0.0.0"));
                }

                acceptor_ = boost::make_shared<boost::asio::ip::tcp::acceptor>(worker_->get_io_context(), endpoint);
                acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
                while (1) {
                    boost::system::error_code ec;
                    auto worker = thread_pool_inst::get_mutable_instance().get_worker(-1);
                    auto tcp_sock = co_await acceptor_->async_accept(worker->get_io_context(), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                    if (!ec) {
                        std::shared_ptr<CONN> client_conn = std::make_shared<CONN>(this, std::move(tcp_sock), worker);
                        client_conn->open();
                    }
                    else {
                        if (!running_) {
                            co_return;
                        }
                        boost::asio::steady_timer timer(co_await this_coro::executor);
                        timer.expires_after(100ms);
                        co_await timer.async_wait(boost::asio::use_awaitable);
                    }
                }
                co_return;
                }), [this](std::exception_ptr exp) {
                    ((void)exp);
                    spdlog::debug("stop tcp server done, port:{}", port_);
                    });
            return 0;
        }

        virtual void stop_listen() {
            running_ = false;
            worker_->dispatch([this] {
                if (acceptor_) {
                    acceptor_->close();
                    acceptor_.reset();
                }
                });
        }
    private:
        uint16_t port_;
        ThreadWorker* worker_;
        std::atomic<bool> running_{ false };
        boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    };
};