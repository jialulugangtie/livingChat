#include <boost/algorithm/string.hpp>

#include "spdlog/spdlog.h"
#include "rtmp_server_session.hpp"
#include "socket_interface.hpp"
#include "rtmp_message.hpp"
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

boost::asio::awaitable<void> RtmpChunkProtocol::send_rtmp_message(const std::vector<std::shared_ptr<RtmpMessage>>& rtmp_msgs) {
    send_sv_bufs_.clear();
    int curr_chunk_header{ 0 };
    for (auto rtmp_msg : rtmp_msgs) {
        int32_t left_size = rtmp_msg->payload_size_;
        size_t cur_pos{ 0 };
        uint8_t fmt = RTMP_CHUNK_FMT_TYPE0;
        while (left_size > 0) {
            size_t buf_pos{ 0 };
            auto prev_chunk = send_chunk_streams_[rtmp_msg->chunk_stream_id_];
            if (prev_chunk) {
                if(prev_chunk->chunk_message_header_.message_stream_id_ == rtmp_msg->message_stream_id_
                    && prev_chunk->chunk_message_header_.message_type_id_ == rtmp_msg->message_type_id_
                    && prev_chunk->chunk_message_header_.message_length_ == rtmp_msg->payload_size_
                    && prev_chunk->chunk_message_header_.timestamp_delta_ == (rtmp_msg->timestamp_ - prev_chunk->chunk_message_header_.timestamp_)) {
                    fmt = RTMP_CHUNK_FMT_TYPE3;
                }
                else if (prev_chunk->chunk_message_header_.message_stream_id_ == rtmp_msg->message_stream_id_
                    && prev_chunk->chunk_message_header_.message_type_id_
                    && prev_chunk->chunk_message_header_.message_length_ == rtmp_msg->payload_size_) {
                    fmt = RTMP_CHUNK_FMT_TYPE2;
                }
                else if (prev_chunk->chunk_message_header_.message_stream_id_ == rtmp_msg->message_stream_id_) {
                    fmt = RTMP_CHUNK_FMT_TYPE1;
                }
                else {
                    fmt = RTMP_CHUNK_FMT_TYPE0;
                }
            }
            else {
                fmt = RTMP_CHUNK_FMT_TYPE0;
            }

            std::shared_ptr<RtmpChunk> chunk = std::make_shared<RtmpChunk>();
            chunk->chunk_message_header_.message_stream_id_ = rtmp_msg->message_stream_id_;
            chunk->chunk_message_header_.message_type_id_ = rtmp_msg->message_type_id_;
            chunk->chunk_message_header_.message_length_ = rtmp_msg->payload_size_;
            chunk->chunk_message_header_.timestamp_ = rtmp_msg->timestamp_;
            if (prev_chunk) {
                chunk->chunk_message_header_.timestamp_delta_ = chunk->chunk_message_header_.timestamp_ - prev_chunk->chunk_message_header_.timestamp_;
            }

            chunk->rtmp_message_ = rtmp_msg;
            if (rtmp_msg->chunk_stream_id_ >= 2 && rtmp_msg->chunk_stream_id_ <= 63) {
                uint8_t d = ((fmt & 0x03) << 6) | (rtmp_msg->chunk_stream_id_ & 0x03f);
                memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, &d, sizeof(d));
                buf_pos++;
            }else if(rtmp_msg->chunk_stream_id_ >= 64 && rtmp_msg->chunk_stream_id_ <= 319){
                uint8_t* p = (uint8_t*)chunk_headers_[curr_chunk_header].get() + buf_pos;
                p[0] = ((fmt & 0x03) << 6) | 0x00;
                p[1] = (rtmp_msg->chunk_stream_id_ - 64) & 0xff;
                buf_pos += 3;
            }
            else if (rtmp_msg->chunk_stream_id_ >= 64 && rtmp_msg->chunk_stream_id_ <= 65599) {
                uint8_t* p = (uint8_t*)chunk_headers_[curr_chunk_header].get() + buf_pos;
                p[0] = ((fmt && 0x03) << 6) | 0x01;
                auto csid = rtmp_msg->chunk_stream_id_ - 64;
                p[1] = (csid % 256) & 0xff;
                p[2] = (csid % 256) & 0xff;
                buf_pos += 3;
            }

            //发送message header
            int this_chunk_payload_size = std::min(out_chunk_size_, left_size);
            bool has_extend_timestamp{ false };
            if (fmt == RTMP_CHUNK_FMT_TYPE0) {
                if (rtmp_msg->timestamp_ >= 0xffffff) {
                    has_extend_timestamp = true;
                }

                if (!has_extend_timestamp) {
                    uint32_t t = htonl(rtmp_msg->timestamp_ & 0xffffff);
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                }
                else {
                    uint32_t t = 0xffffffff;
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                }
                buf_pos += 3;

                int32_t t = htonl(rtmp_msg->payload_size_);
                memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                buf_pos += 3;

                memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, &rtmp_msg->message_stream_id_, 1);
                buf_pos += 1;

                t = htonl(rtmp_msg->message_stream_id_);
                memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, &t, 4);
                buf_pos += 4;

                if (has_extend_timestamp) {
                    uint32_t t = htonl(rtmp_msg->timestamp_);
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t, 4);
                    buf_pos += 4;
                }
            }
            else if (fmt == RTMP_CHUNK_FMT_TYPE1) {
                uint32_t timestamp_delta = rtmp_msg->timestamp_ - prev_chunk->rtmp_message_->timestamp_;
                if (timestamp_delta >= 0xffffff) {
                    has_extend_timestamp = true;
                }
                if (!has_extend_timestamp) {
                    uint32_t t = htonl(timestamp_delta);
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                }
                else {
                    uint32_t t = 0xffffffff;
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                }
                buf_pos += 3;
                int32_t t = htonl(rtmp_msg->payload_size_);
                memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                buf_pos += 3;

                int32_t t = htonl(rtmp_msg->payload_size_);
                memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                buf_pos += 3;

                memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, &rtmp_msg->message_type_id_, 1);
                buf_pos += 1;
                if (has_extend_timestamp) {
                    uint32_t t = htonl(timestamp_delta);
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t, 4);
                    buf_pos += 4;
                }
            }
            else if (fmt == RTMP_CHUNK_FMT_TYPE2) {
                uint32_t timestamp_delta = rtmp_msg->timestamp_ - prev_chunk->rtmp_message_->timestamp_;
                if (timestamp_delta >= 0xffffff) {
                    has_extend_timestamp = true;
                }

                if (!has_extend_timestamp) {
                    uint32_t t = htonl(timestamp_delta);
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                }
                else {
                    uint32_t t = 0xffffffff;
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t + 1, 3);
                }
                buf_pos += 3;

                if (has_extend_timestamp) {
                    uint32_t t = htonl(timestamp_delta);
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t, 4);
                    buf_pos += 4;
                }
            }
            else if (fmt == RTMP_CHUNK_FMT_TYPE3) {
                uint32_t timestamp_delta = rtmp_msg->timestamp_ - prev_chunk->rtmp_message_->timestamp_;
                if (timestamp_delta >= 0xffffff) {
                    has_extend_timestamp = true;
                }

                if (has_extend_timestamp){
                    uint32_t t = htonl(timestamp_delta);
                    memcpy(chunk_headers_[curr_chunk_header].get() + buf_pos, (uint8_t*)&t, 4);
                    buf_pos += 4;
                }
            }

            send_sv_bufs_.push_back(boost::asio::const_buffer((char*)chunk_headers_[curr_chunk_header].get(), buf_pos));

            send_sv_bufs_.push_back(boost::asio::const_buffer((char*)rtmp_msg->payload_ + cur_pos, this_chunk_payload_size));

            curr_chunk_header++;
            left_size -= this_chunk_payload_size;
            cur_pos += this_chunk_payload_size;

            send_chunk_streams_[rtmp_msg->chunk_stream_id_] = chunk;

            
        }

    }
}


boost::asio::awaitable<bool> RtmpServerSession::on_recv_rtmp_message(std::shared_ptr<RtmpMessage>& rtmp_msg){
    spdlog::info("got a rtmp message, type:{}", rtmp_msg->message_type_id_);
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