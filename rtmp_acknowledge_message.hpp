#include<memory>
namespace mms {
	class RtmpMessage;
	class RtmpAcknwledgeMessage {
	public:
		RtmpAcknwledgeMessage(size_t s);
		RtmpAcknwledgeMessage();
		int32_t decode(std::shared_ptr<RtmpMessage> rtmp_msg);
		std::shared_ptr<RtmpMessage> encode() const;
	public:
		size_t acknowledge_{ 0 };
	};
}