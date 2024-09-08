#include "socket_interface.hpp"
using namespace mms;
std::atomic<uint64_t> SocketInterface::socket_id_ = { 0 };
SocketInterface::SocketInterface() {

}

SocketInterface::~SocketInterface() {

}

void SocketInterface::set_session(std::shared_ptr<Session> session) {
    session_ = session;
}

std::shared_ptr<Session> SocketInterface::get_session() {
    return session_;
}

void SocketInterface::clear_session() {
    session_ = nullptr;
}