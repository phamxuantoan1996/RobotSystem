#pragma once
#include <functional>
#include <atomic>
#include <system_error>
#include <thread>
#include <utility>

namespace navigator::application::services {
    struct NavigatorReconnectConfig {
        int maxRetries = 5;
        int retryIntervalMs = 3000;
    };

    class NavigatorReconnectService {
        public:
            // callback for reconnection
            using ConnectFunction = std::function<std::error_code()>;
            // callback for reconnection successfully
            using OnSuccess = std::function<void()>;
            // callback
            using OnGiveUp = std::function<void(int attempts)>;

            explicit NavigatorReconnectService(NavigatorReconnectConfig config) : config_(std::move(config)) {}

            ~NavigatorReconnectService();

            // Non-blocking, background thread
            void startAsync(ConnectFunction connect, OnSuccess onSuccess, OnGiveUp onGiveUp);

            // block ultil thread finished
            void waitForCompletion();

            // request stop
            void stop();

            // Disable copy
            NavigatorReconnectService(const NavigatorReconnectService&) = delete;
            NavigatorReconnectService& operator=(const NavigatorReconnectService&) = delete;
            
            NavigatorReconnectService(NavigatorReconnectService&& other) = delete;
            NavigatorReconnectService& operator=(NavigatorReconnectService&& other) = delete;

        private:
            NavigatorReconnectConfig config_;
            std::atomic<bool> running_{false};
            std::thread reconnectThread_;
    };
}