#pragma once
#include <string>
#include <optional>
#include <unordered_map>
#include <memory>

#include "session.hpp"
namespace mms {
    class StreamSession : public Session {
    public:
        StreamSession(ThreadWorker* worker);
        virtual ~StreamSession();
    public:
        void set_session_info(const std::string& domain, const std::string& app_name, const std::string& stream_name);
        void set_session_info(const std::string_view& domain, const std::string_view& app_name, const std::string_view& stream_name);

        const std::string& get_domain() const {
            return domain_;
        }

        const std::string& get_app_name() const {
            return app_name_;
        }

        const std::string& get_stream_name() const {
            return stream_name_;
        }

    protected:
        std::string domain_;
        std::string app_name_;
        std::string stream_name_;
    };
};