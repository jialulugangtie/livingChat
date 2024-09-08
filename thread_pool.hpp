#pragma once

//#include <sys/prctl.h>

#include <functional>
#include <thread>
#include <memory>
#include <boost/atomic.hpp>

#include <boost/serialization/singleton.hpp> 

#include "thread_worker.hpp"
#define RAND_WORKER -1
namespace mms {
    class ThreadPool {
    public:
        ThreadPool() : running_(false), using_worker_idx_(0) {

        }

        virtual ~ThreadPool() {
            stop();
        }

        void start(int cpu_count) {
            if (running_) {
                return;
            }
            running_ = true;
            workers_.reserve(cpu_count);
            for (int i = 0; i < cpu_count; i++) {
                ThreadWorker* w = new ThreadWorker();
                w->set_cpu_core(i);
                w->start();
                workers_.emplace_back(w);
            }
        }

        void stop() {
            if (!running_) {
                return;
            }
            running_ = false;
            for (auto& w : workers_) {
                w->stop();
                delete w;
            }
        }

        ThreadWorker* get_worker(int cpu_num) {
            if (cpu_num == -1) {
                uint32_t idx = using_worker_idx_ % workers_.size();
                using_worker_idx_++;
                return workers_[idx];
            }
            return workers_[cpu_num];
        }

        std::vector<ThreadWorker*>& get_all_workers() {
            return workers_;
        }
    private:
        boost::atomic_bool running_;
        boost::atomic_uint64_t using_worker_idx_;
        std::vector<ThreadWorker*> workers_;
    };

    typedef boost::serialization::singleton<ThreadPool> thread_pool_inst;
};