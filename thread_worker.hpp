#pragma once

#pragma once
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>

namespace mms {
    class ThreadWorker;
    class ThreadWorker {
    public:
        using Task = std::function<void()>;
        ThreadWorker();
        virtual ~ThreadWorker();
        // 设置在哪个cpu核心上运行
        void set_cpu_core(int cpu_core);
        int get_cpu_core();

        template<typename F, typename ...ARGS>
        void post(F&& f, ARGS &&...args) {
            io_context_.post(std::bind(f, std::forward<ARGS>(args)...));
        }

        template<typename F, typename ...ARGS>
        void dispatch(F&& f, ARGS &&...args) {
            io_context_.dispatch(std::bind(f, std::forward<ARGS>(args)...));
        }

        void start();
        void stop();
        boost::asio::io_context& get_io_context();
    private:
        void work();
        int cpu_core_;
        boost::asio::io_context io_context_;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
        std::jthread thread_;
        std::atomic_bool running_;
    };

};