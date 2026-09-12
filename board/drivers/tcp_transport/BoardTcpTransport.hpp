#pragma once
#include "../board/ports/IBoardTransport.hpp"
#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>

namespace board::drivers::tcp_transport {
    struct TcpTransportConfig {
        std::string host = "127.0.0.1";
        uint16_t port = 3000;
        uint32_t timeout = 5000; // miliseconds
    };

    class BoardTcpTransport : public board::ports::IBoardTransport {
        public:
        explicit BoardTcpTransport(TcpTransportConfig config) : config_(std::move(config))
        {
            std::cout << "Tcp transport init with : \n";
            std::cout << "Host : " << config_.host << std::endl;
            std::cout << "Port : " << config_.port << std::endl;
        }

        ~BoardTcpTransport()
        {
            disconnect();
        }

        BoardTcpTransport(const BoardTcpTransport& other) = delete;
        BoardTcpTransport& operator=(const BoardTcpTransport&) = delete;

        std::error_code connect() override;
        std::error_code reconnect(int delay_ms) override;
        bool isConnected() const override;
        void disconnect() override;

        std::error_code readExactly(std::string& data, size_t n, int timeout_ms = 1000) override;
        std::error_code readUntil(std::string& data, char delimiter, int timeout_ms = 1000) override;
        std::error_code write(const std::string& data, int timeout_ms) override;

        private:
            int socketFd_ = -1;
            TcpTransportConfig config_;
            std::atomic<bool> connected_{false};

            mutable std::mutex portMutex_;
    };
}