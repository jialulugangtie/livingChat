#include "rtmp_abort_message.hpp"
#include "rtmp_message.hpp"
#include "rtmp_chunk_protocol.hpp"

using namespace mms;
int32_t RtmpAbortMessage::decode(std::shared_ptr<RtmpMessage> rtmp_msg) {
	uint8_t* payload = rtmp_msg->payload_;
	int32_t len = rtmp_msg->payload_size_;
	if (len < 4) return -1;

	chunk_id_ = ntohl(*(uint32_t*)payload);
	return 4;
}

std::shared_ptr<RtmpMessage> RtmpAbortMessage::encode() {
	std::shared_ptr<RtmpMessage> rtmp_msg = std::make_shared<RtmpMessage>(sizeof(chunk_id_));
	rtmp_msg->chunk_stream_id_ = RTMP_CHUNK_ID_PROTOCOL_CONTROL_MESSAGE;
	rtmp_msg->timestamp_ = 0;
	rtmp_msg->message_type_id_ = RTMP_CHUNK_MESSAGE_TYPE_ABORT_MESSAGE;
	rtmp_msg->message_stream_id_ = RTMP_CHUNK_MESSAGE_ID_PROTOCOL_CONTROL;

	*(uint32_t*)rtmp_msg->payload_ = htonl(chunk_id_);
	rtmp_msg->payload_size_ = sizeof(chunk_id_);
	return rtmp_msg;
}