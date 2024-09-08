#include <iostream>
#include <span>
#include <variant>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/redirect_error.hpp>

#include "tcp_socket.hpp"

using namespace boost::asio::experimental::awaitable_operators;
namespace mms {
    TcpSocket::TcpSocket(TcpSocketHandler* handler, boost::asio::ip::tcp::socket sock, ThreadWorker* worker) :
        worker_(worker),
        socket_(std::move(sock)),
        handler_(handler),
        recv_timeout_timer_(worker->get_io_context()),
        send_timeout_timer_(worker->get_io_context())
    {
        id_ = socket_id_++;
    }

    TcpSocket::TcpSocket(TcpSocketHandler* handler, ThreadWorker* worker) :worker_(worker),
        socket_(worker->get_io_context()),
        handler_(handler),
        recv_timeout_timer_(worker->get_io_context()),
        send_timeout_timer_(worker->get_io_context())
    {
        id_ = socket_id_++;
    }

    TcpSocket::~TcpSocket() {
    }

    void TcpSocket::open() {
        handler_->on_tcp_socket_open(std::static_pointer_cast<TcpSocket>(shared_from_this()));
    }

    boost::asio::awaitable<bool> TcpSocket::connect(const std::string& ip, uint16_t port, int32_t timeout_ms)
    {
        ((void)timeout_ms);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);
        boost::system::error_code ec;
        co_await socket_.async_connect(endpoint, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        if (ec) {
            co_return false;
        }
        open();
        co_return true;
    }

    boost::asio::awaitable<bool> TcpSocket::send(const uint8_t* data, size_t len, int32_t timeout_ms) {
        std::size_t s;
        if (timeout_ms > 0) {
            send_timeout_timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
            // boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
            // timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
            std::variant<bool, std::monostate> results;
            results = co_await(
                [data, len, this]()->boost::asio::awaitable<bool> {
                    boost::system::error_code ec;
                    size_t pos = 0;
                    while (pos < len) {
                        auto size = co_await socket_.async_send(boost::asio::buffer(data + pos, len - pos), 0, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                        if (ec) {
                            co_return false;
                        }
                        pos += size;
                    }
                    co_return true;
                }() ||
                    send_timeout_timer_.async_wait(boost::asio::use_awaitable)
                    );

            if (results.index() == 1) {//超时
                co_return false;
            }

            if (!std::get<0>(results)) {
                co_return false;
            }
        }
        else {
            boost::system::error_code ec;
            size_t pos = 0;
            while (pos < len) {
                s = co_await socket_.async_send(boost::asio::buffer(data + pos, len - pos), 0, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if (ec) {
                    co_return false;
                }
                pos += s;
            }
        }

        out_bytes_ += len;
        co_return true;
    }

    boost::asio::awaitable<bool> TcpSocket::send(std::vector<boost::asio::const_buffer>& bufs, int32_t timeout_ms) {
        size_t len = 0;
        for (auto& buf : bufs) {
            len += buf.size();
        }

        if (timeout_ms > 0) {
            send_timeout_timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
            // boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
            // timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
            std::variant<bool, std::monostate> results;
            results = co_await(
                [&bufs, len, this]()->boost::asio::awaitable<bool> {
                    boost::system::error_code ec;
                    size_t sended_bytes = 0;
                    std::span sbufs(bufs);
                    int32_t s;
                    while (sended_bytes < len) {
                        s = co_await socket_.async_send(sbufs, 0, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                        if (ec) {
                            co_return false;
                        }

                        sended_bytes += s;
                        size_t i = 0;
                        for (i = 0; s > 0 && i < sbufs.size(); ) {
                            if ((int32_t)sbufs[i].size() <= s) {
                                s -= (int32_t)sbufs[i].size();
                                i++;
                            }
                            else {
                                sbufs[i] += s;
                                break;
                            }
                        }

                        if (sended_bytes < len && i > 0) {
                            sbufs = sbufs.subspan(i);
                        }
                    }
                    co_return true;
                }() ||
                    send_timeout_timer_.async_wait(boost::asio::use_awaitable)
                    );

            if (results.index() == 1) {//超时
                co_return false;
            }

            if (!std::get<0>(results)) {
                co_return false;
            }
        }
        else {
            boost::system::error_code ec;
            size_t sended_bytes = 0;
            std::span sbufs(bufs);
            int32_t s;
            while (sended_bytes < len) {
                s = co_await socket_.async_send(sbufs, 0, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if (ec) {
                    co_return false;
                }

                sended_bytes += s;
                size_t i = 0;
                for (i = 0; s > 0 && i < sbufs.size(); ) {
                    if ((int32_t)sbufs[i].size() <= s) {
                        s -= (int32_t)sbufs[i].size();
                        i++;
                    }
                    else {
                        sbufs[i] += s;
                        break;
                    }
                }

                if (sended_bytes < len && i > 0) {
                    sbufs = sbufs.subspan(i);
                }
            }
        }

        out_bytes_ += len;
        co_return true;
    }

    boost::asio::awaitable<bool> TcpSocket::recv(uint8_t* data, size_t len, int32_t timeout_ms) {
        if (timeout_ms > 0) {
            recv_timeout_timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
            // boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
            // timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
            std::variant<bool, std::monostate> results;
            results = co_await(
                [data, len, this]()->boost::asio::awaitable<bool> {
                    size_t pos = 0;
                    boost::system::error_code ec;
                    while (pos < len) {
                        auto s = co_await socket_.async_receive(boost::asio::buffer(data + pos, len - pos), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                        if (ec) {
                            co_return false;
                        }
                        pos += s;
                    }
                    co_return true;
                }() ||
                    recv_timeout_timer_.async_wait(boost::asio::use_awaitable)
                    );

            if (results.index() == 1) {//超时
                co_return false;//考虑下返回码的问题,error应该需要包装下，但还得考虑性能
            }

            if (!std::get<0>(results)) {
                co_return false;
            }
        }
        else {
            size_t pos = 0;
            boost::system::error_code ec;
            while (pos < len) {
                auto s = co_await socket_.async_receive(boost::asio::buffer(data + pos, len - pos), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
                if (ec) {
                    co_return false;
                }
                pos += s;
            }
        }

        in_bytes_ += len;
        co_return true;
    }

    boost::asio::awaitable<int32_t> TcpSocket::recv_some(uint8_t* data, size_t len, int32_t timeout_ms) {
        boost::system::error_code ec;
        std::size_t s;
        if (timeout_ms > 0) {
            recv_timeout_timer_.expires_from_now(std::chrono::milliseconds(timeout_ms));
            // boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
            // timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
            std::variant<std::size_t, std::monostate> results = co_await(
                socket_.async_read_some(boost::asio::buffer(data, len), boost::asio::redirect_error(boost::asio::use_awaitable, ec)) ||
                recv_timeout_timer_.async_wait(boost::asio::use_awaitable)
                );

            if (results.index() == 1) {//超时
                co_return -1;
            }
            s = std::get<0>(results);
        }
        else {
            s = co_await socket_.async_read_some(boost::asio::buffer(data, len), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }

        if (ec) {
            co_return -2;
        }
        in_bytes_ += s;
        co_return s;
    }

    void TcpSocket::close() {
        if (closed_.test_and_set(std::memory_order_acquire)) {
            return;
        }

        if (handler_) {
            handler_->on_tcp_socket_close(std::static_pointer_cast<TcpSocket>(shared_from_this()));
        }
        socket_.close();
    }


};