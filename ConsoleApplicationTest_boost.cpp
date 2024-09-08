#include <iostream>
#include <boost/asio.hpp>
#include "thread_pool.hpp"
#include<thread>
#include "rtmp_server.hpp"
#include "spdlog/spdlog.h"
#include<chrono>
using namespace mms;
int main(int argc, char* argv[]) {
    thread_pool_inst::get_mutable_instance().start(std::thread::hardware_concurrency());
    RtmpServer rtmp_server(thread_pool_inst::get_mutable_instance().get_worker(RAND_WORKER));
    if (!rtmp_server.start(1935))
    {
        spdlog::error("start rtmp server failed");
        return -1;
    }

    while (1) {
        //sleep(1000);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}