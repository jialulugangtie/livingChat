#include "stream_session.hpp"
#include "thread_worker.hpp"

using namespace mms;

StreamSession::StreamSession(ThreadWorker* worker) : Session(worker) {
}

StreamSession::~StreamSession() {

}

void StreamSession::set_session_info(const std::string& domain, const std::string& app_name, const std::string& stream_name) {
    domain_ = domain;
    app_name_ = app_name;
    stream_name_ = stream_name;
    session_name_ = domain_ + "/" + app_name_ + "/" + stream_name_;
}

void StreamSession::set_session_info(const std::string_view& domain, const std::string_view& app_name, const std::string_view& stream_name) {
    domain_ = domain;
    app_name_ = app_name;
    stream_name_ = stream_name;
    session_name_ = domain_ + "/" + app_name_ + "/" + stream_name_;
}