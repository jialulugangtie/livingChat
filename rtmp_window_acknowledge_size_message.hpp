#pragma once
#include<memory>
namespace mms {
	class RtmpMessage;
	class RtmpWindowAcknwledgeSizeMessage {
	public:
	public:
		RtmpWindowAcknwledgeSizeMessage(size_t s);
		RtmpWindowAcknwledgeSizeMessage();
		int32_t decode(std::shared_ptr<RtmpMessage> rtmp_msg);
		std::shared_ptr<RtmpMessage> encode() const;

	public:
		size_t acknowledge_window_size_{ 5 * 1024 * 1024 };
	};
}