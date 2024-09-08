#include "session.hpp"
#include "thread_worker.hpp"

using namespace mms;

std::atomic<uint64_t> Session::session_id_ = 0;

Session::Session(ThreadWorker* worker) : worker_(worker) {
    id_ = session_id_++;
}

Session::~Session() {

}

std::optional<std::reference_wrapper<const std::string>> Session::get_param(const std::string& key) {
    auto it = params_.find(key);
    if (it == params_.end()) {
        return std::nullopt;
    }
    return it->second;
}