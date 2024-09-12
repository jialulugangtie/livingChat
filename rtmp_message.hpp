#pragma once
#include<memory>
#include<string_view>

namespace mms {
	class RtmpChunk;
	class RtmpMessage {
	public:
		RtmpMessage(int32_t payload_size);
		virtual ~RtmpMessage();
		inline uint8_t get_message_type() {
			return message_type_id_;
		}

		std::string_view get_payload();

	public:
		uint8_t* payload_{ nullptr };
		int32_t payload_size_{ 0 };
		int32_t chunk_stream_id_{ 0 };
		int32_t timestamp_{ 0 };
		int32_t message_type_id_{ 0 };
		int32_t message_stream_id_{ 0 };
	};


}