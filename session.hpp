#pragma once
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>
#include <optional>
namespace mms {
    class ThreadWorker;

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(ThreadWorker* worker);
        virtual ~Session();
        virtual void service() = 0;
        virtual void close() = 0;

        ThreadWorker* get_worker() const {
            return worker_;
        }

        std::string& get_session_name() {
            return session_name_;
        }

        void set_session_type(const std::string& session_type) {
            session_type_ = session_type;
        }

        const std::string& get_session_type() {
            return session_type_;
        }

        void set_param(const std::string& key, const std::string& val) {
            params_[key] = val;
        }

        std::optional<std::reference_wrapper<const std::string>> get_param(const std::string& key);
    protected:
        uint64_t id_;
        std::string session_name_;
        std::string session_type_;
        std::unordered_map<std::string, std::string> params_;
        ThreadWorker* worker_;
    private:
        static std::atomic<uint64_t> session_id_;
    };

};