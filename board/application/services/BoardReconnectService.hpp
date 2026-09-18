#pragma once
#include <functional>
#include <atomic>
#include <system_error>
#include <thread>
#include <utility>

namespace board::application::services {
    struct BoardReconnectConfig {
        int maxRetries = 5;
        int retryIntervalMs = 3000;
    };

    class BoardReconnectService {
        public:
            // callback for reconnection
            using ConnectFunction = std::function<std::error_code()>;
            // callback for reconnection successfully
            using OnSuccess = std::function<void()>;
            // callback
            using OnGiveUp = std::function<void(int attempts)>;

            explicit BoardReconnectService(BoardReconnectConfig config) : config_(std::move(config)) {}

            ~BoardReconnectService();

            // Non-blocking, background thread
            void startAsync(ConnectFunction connect, OnSuccess onSuccess, OnGiveUp onGiveUp);

            // block ultil thread finished
            void waitForCompletion();

            // request stop
            void stop();

            // Disable copy
            BoardReconnectService(const BoardReconnectService&) = delete;
            BoardReconnectService& operator=(const BoardReconnectService&) = delete;
            
            BoardReconnectService(BoardReconnectService&& other) = delete;
            BoardReconnectService& operator=(BoardReconnectService&& other) = delete;

        private:
            BoardReconnectConfig config_;
            std::atomic<bool> running_{false};
            std::thread reconnectThread_;
    };
}