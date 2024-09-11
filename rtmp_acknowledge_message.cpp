#include "rtmp_acknowledge_message.hpp"
#include "rtmp_message.hpp"
#include "rtmp_chunk_protocol.hpp"

using namespace mms;

RtmpAcknwledgeMessage::RtmpAcknwledgeMessage(size_t s) : acknowledge_(s) {

}

RtmpAcknwledgeMessage::RtmpAcknwledgeMessage() {

}

int32_t RtmpAcknwledgeMessage::decode(std::shared_ptr<RtmpMessage> rtmp_msg) {
	uint8_t* payload = rtmp_msg->payload_;
	int32_t len = rtmp_msg->payload_size_;
	if (len < 4) {
		return -1;
	}

	acknowledge_ = ntohl(*(uint32_t*)payload);
	return 4;
}

std::shared_ptr<RtmpMessage> RtmpAcknwledgeMessage::encode() const {
	std::shared_ptr<RtmpMessage> rtmp_msg = std::make_shared<RtmpMessage>(4);
	rtmp_msg->chunk_stream_id_ = RTMP_CHUNK_ID_PROTOCOL_CONTROL_MESSAGE;
	rtmp_msg->timestamp_ = 0;
	rtmp_msg->message_type_id_ = RTMP_CHUNK_MESSAGE_TYPE_ACKNOWLEDGEMENT;
	rtmp_msg->message_stream_id_ = RTMP_CHUNK_MESSAGE_ID_PROTOCOL_CONTROL;

	*(uint32_t*)rtmp_msg->payload_ = htonl(acknowledge_);
	rtmp_msg->payload_size_ = 4;
	return rtmp_msg;
}