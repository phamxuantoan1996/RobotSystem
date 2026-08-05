#include "NavigatorReconnectService.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

namespace navigator::application::services {
    NavigatorReconnectService::~NavigatorReconnectService()
    {
        stop();
        waitForCompletion();
    }

    void NavigatorReconnectService::startAsync(ConnectFunction connect, OnSuccess onSuccess, OnGiveUp onGiveUp)
    {
        bool expected = false;
        if(!running_.compare_exchange_strong(expected,true))
        {
            return;
        }

        // waiting for old thread to end
        if(reconnectThread_.joinable())
        {
            reconnectThread_.join();
        }
            
        reconnectThread_ = std::thread([this,connect = std::move(connect),onSuccess = std::move(onSuccess),onGiveUp = std::move(onGiveUp)](){
            int attempts = 0;
            while(running_ && attempts < config_.maxRetries)
            {
                attempts++;
                std::cout << "[Reconnect] attempt " << attempts << "/" << config_.maxRetries << "\n";
                auto ec = connect();
                std::cout << "================> reconnect pass 0\n";
                if(!ec) // reconnection successfully
                {
                    running_ = false;
                    if(onSuccess)
                    {
                        onSuccess();
                    }
                    return;
                }
                std::cout << "================> reconnect pass 1\n";
                const int checkIntervalMs = 100;
                int remainingMs = config_.retryIntervalMs;
                while(running_ && remainingMs > 0)
                {
                    int sleepTime = std::min(checkIntervalMs,remainingMs);
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                    remainingMs -= sleepTime;
                }
                std::cout << "================> reconnect pass 2\n";

            }

            // reconnection failure
            if(running_)
            {
                running_ = false;
                if(onGiveUp)
                {
                    onGiveUp(attempts);
                }
            }
        });
    }

    void NavigatorReconnectService::stop()
    {
        running_ = false;
    }

    void NavigatorReconnectService::waitForCompletion()
    {
        if(reconnectThread_.joinable())
        {
            reconnectThread_.join();
        }
    }
}