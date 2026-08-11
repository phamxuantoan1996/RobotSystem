#pragma once
#include "../board/ports/IBoardTransport.hpp"

#include <atomic>
#include <iostream>
#include <string>
#include <cstring>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <mutex>

namespace board::drivers::serial_transport {
    struct SerialTransportConfig
    {
        /* data */
        std::string serial_port = "/dev/ttyUSB0";
        uint32_t baudrate = 115200;
        uint32_t timeout = 5000; // milisecond
        // uint32_t timePoll = 100; // milisecond
    };

    class BoardSerialTransport : public board::ports::IBoardTransport
    {
        public:
            explicit BoardSerialTransport(SerialTransportConfig config) : config_(std::move(config))
            {
                std::cout << "Serial transport init with : \n";
                std::cout << "Serial port name : " << config_.serial_port << std::endl;
                std::cout << "Serial baud rate : " << config_.baudrate << std::endl;
            }

            ~BoardSerialTransport()
            {
                disconnect();
            }

            BoardSerialTransport(const BoardSerialTransport& other) = delete;
            BoardSerialTransport& operator=(const BoardSerialTransport&) = delete;

            std::error_code connect() override;
            std::error_code reconnect(int delay_ms) override;
            bool isConnected() const override;
            void disconnect() override;

            std::error_code readExactly(std::string& data, size_t n, int timeout_ms = 1000) override;
            std::error_code readUntil(std::string& data, char delimiter, int timeout_ms = 1000) override;
            std::error_code write(const std::string& data, int timeout_ms) override;

        private:
            int serialFd_ = -1;
            SerialTransportConfig config_;
            std::atomic<bool> connected_;
            mutable std::mutex portMutex_;

            speed_t getBaudRateConstant();
            std::error_code configurePort();
    };
}