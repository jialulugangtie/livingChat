#pragma once
#include<string>
#include<memory>

namespace mms {
	class RtmpMessage;
	class RtmpAbortMessage {
	public:
		int32_t decode(std::shared_ptr<RtmpMessage> rtmp_msg);
		std::shared_ptr<RtmpMessage> encode();

	public:
		uint32_t chunk_id_;
	};
}