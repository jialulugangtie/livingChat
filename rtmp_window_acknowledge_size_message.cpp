#include "rtmp_window_acknowledge_size_message.hpp"
#include "rtmp_message.hpp"
#include "rtmp_chunk_protocol.hpp"

using namespace mms;
RtmpWindowAcknwledgeSizeMessage::RtmpWindowAcknwledgeSizeMessage(size_t s) : acknowledge_window_size_(s)
{
}

RtmpWindowAcknwledgeSizeMessage::RtmpWindowAcknwledgeSizeMessage() {

}

int32_t RtmpWindowAcknwledgeSizeMessage::decode(std::shared_ptr<RtmpMessage> rtmp_msg) {
	uint8_t* payload = rtmp_msg->payload_;
	int32_t len = rtmp_msg->payload_size_;
	if (len < 4) {
		return -1;
	}

	acknowledge_window_size_ = ntohl(*(uint32_t*)payload);
	return 4;
}

std::shared_ptr<RtmpMessage> RtmpWindowAcknwledgeSizeMessage::encode() const {
	std::shared_ptr<RtmpMessage> rtmp_msg = std::make_shared<RtmpMessage>(4);
	rtmp_msg->chunk_stream_id_ = RTMP_CHUNK_ID_PROTOCOL_CONTROL_MESSAGE;
	rtmp_msg->timestamp_ = 0;
	rtmp_msg->message_type_id_ = RTMP_CHUNK_MESSAGE_TYPE_WINDOW_ACK_SIZE;
	rtmp_msg->message_stream_id_ = RTMP_CHUNK_MESSAGE_ID_PROTOCOL_CONTROL;

	*(int32_t*)rtmp_msg->payload_ = htonl(acknowledge_window_size_);
	rtmp_msg->payload_size_ = 4;
	return rtmp_msg;
}
