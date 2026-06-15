#pragma once

#include <string>
#include <atomic>
#include <thread>

class HTTPServer {
    public:
        ~HTTPServer();
        bool Start(int port, const std::string& rootDir);
        void Stop();

    private:
        void acceptLoop();

        std::thread thread_;
        std::atomic<bool> running_{false};
        std::string rootDir_;
        int port_ = 0;
        uintptr_t listenSocket_ = ~uintptr_t(0);
};
