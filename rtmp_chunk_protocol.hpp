#pragma once
#include<functional>
#include<unordered_map>
#include<memory>
#include<vector>
#include<boost/asio/experimental/awaitable_operators.hpp>
#include<boost/asio/experimental/channel.hpp>
#include<boost/asio/experimental/concurrent_channel.hpp>
#include<boost/asio/awaitable.hpp>
#include "rtmp_conn.hpp"
#include "rtmp_set_chunk_size_message.hpp"
#include "rtmp_abort_message.hpp"


namespace mms {
	class SocketInterface;
	class RtmpMessage;

#define RTMP_CHUNK_FMT_TYPE0 0
#define RTMP_CHUNK_FMT_TYPE1 1
#define RTMP_CHUNK_FMT_TYPE2 2
#define RTMP_CHUNK_FMT_TYPE3 3

#define RTMP_CHUNK_ID_PROTOCOL_CONTROL_MESSAGE 2
#define RTMP_CHUNK_MESSAGE_ID_PROTOCOL_CONTROL 0

#define RTMP_CHUNK_MESSAGE_TYPE_SET_CHUNK_SIZE 1
#define RTMP_CHUNK_MESSAGE_TYPE_ABORT_MESSAGE 2
#define RTMP_CHUNK_MESSAGE_TYPE_ACKNOWLEDGEMENT 3
#define RTMP_CHUNK_MESSAGE_TYPE_WINDOW_ACK_SIZE 5
#define RTMP_CHUNK_MESSAGE_TYPE_SET_PEER_BANDWIDTH 6

	class ChunkMesageHeader {
	public:
		int8_t fmt_{ 0 };
		int32_t timestamp_delta_{ 0 };
		int32_t timestamp_{ 0 };
		int32_t message_length_{ 0 };
		int32_t message_type_id_{ 0 };
		int32_t message_stream_id_{ 0 };

		void clear() {
			fmt_ = 0;
			timestamp_delta_ = 0;
			timestamp_ = 0;
			message_length_ = 0;
			message_type_id_ = 0;
			message_stream_id_ = 0;
		}
	};

	class RtmpChunk {
	public:
		void clear() {
			chunk_message_header_.clear();
			rtmp_message_ = nullptr;
		}

	public:
		ChunkMesageHeader chunk_message_header_;
		std::shared_ptr<RtmpMessage> rtmp_message_;
	};

	class RtmpChunkProtocol {
	public:
		RtmpChunkProtocol(std::shared_ptr<SocketInterface> conn);
		virtual ~RtmpChunkProtocol();

	public:
		boost::asio::awaitable<int32_t> cycle_recv_rtmp_message(const std::function<boost::asio::awaitable<bool>(std::shared_ptr<RtmpMessage>&)>& recv_handler);
		
		inline size_t get_out_chunk_size() {
			return out_chunk_size_;
		}

		inline void set_out_chunk_size(size_t s) {
			out_chunk_size_ = s;
		}

		inline void set_in_chunk_size(int32_t s) {
			in_chunk_size_ = s;
		}

		inline int32_t get_in_chunk_size() {
			return in_chunk_size_;
		}

		boost::asio::awaitable<bool> send_rtmp_messages(const std::vector<std::shared_ptr<RtmpMessage>>& rtmp_msgs);

		int64_t get_last_ack_bytes() {
			return last_ack_bytes_;
		}

		inline void set_last_ack_bytes(int64_t v) {
			last_ack_bytes_ = v;
		}

		int64_t get_in_window_acknowledge_size() {
			return in_window_acknowledge_size_;
		}

		inline void set_in_window_acknowledge_size(int64_t v) {
			in_window_acknowledge_size_ = v;
		}

		bool is_protocol_control_message(std::shared_ptr<RtmpMessage> msg);
		boost::asio::awaitable<bool> handle_protocol_control_message(std::shared_ptr<RtmpMessage> msg);

	private:
		boost::asio::awaitable<int32_t> process_recv_buffer();

		bool handle_set_chunk_size(std::shared_ptr<RtmpMessage> msg);
		bool handle_abort(std::shared_ptr<RtmpMessage> msg);
		bool handle_window_acknowledge_size(std::shared_ptr<RtmpMessage> msg);

	private:
		std::shared_ptr<SocketInterface> conn_;
		std::function<boost::asio::awaitable<bool>(std::shared_ptr<RtmpMessage>&)> recv_handler_;

		uint8_t* recv_buffer_{ nullptr };
		uint32_t recv_len_{ 0 };

		int32_t in_chunk_size_{ 128 };
		int32_t out_chunk_size_{ 128 };

		int64_t in_window_acknowledge_size_{ 5 * 1024 * 1024 };
		int64_t last_ack_bytes_{ 0 };

		std::vector<std::unique_ptr<char[]>> chunk_headers_;
		std::vector<std::string_view> chunk_headers_sv_;

		std::unordered_map<uint32_t, std::shared_ptr<RtmpChunk>> send_chunk_streams_;
		std::vector<boost::asio::const_buffer> send_sv_bufs_;

		std::unordered_map<uint32_t, std::shared_ptr<RtmpChunk>> recv_chunk_streams_;
		std::unordered_map<uint32_t, std::shared_ptr<RtmpChunk>> recv_chunk_cache_;
	};






}