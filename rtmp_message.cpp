#include<string.h>
#include "rtmp_message.hpp"

using namespace mms;


RtmpMessage::RtmpMessage(int32_t payload_size) {
	payload_ = new uint8_t[payload_size];
	memset(payload_, 0, payload_size);
}

RtmpMessage::~RtmpMessage() {
	if (payload_) {
		delete payload_;
		payload_ = nullptr;
	}
	payload_size_ = 0;
}

std::string_view RtmpMessage::get_payload() {
	return std::string_view((char*)payload_, payload_size_);
}
