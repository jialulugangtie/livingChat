#pragma once
#include<string>
#include<memory>

namespace mms {
	class RtmpMessage;
	class RtmpSetChunkSizeMessage {
	public:
		RtmpSetChunkSizeMessage(size_t s);
		RtmpSetChunkSizeMessage();
		int32_t decode(std::shared_ptr<RtmpMessage> rtmp_msg);
		std::shared_ptr<RtmpMessage> encode() const;

	public:
		int32_t chunk_size_;
	};
}