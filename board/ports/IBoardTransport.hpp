#pragma once
#include <system_error>
namespace board::ports {

    class IBoardTransport {
    public:
        virtual std::error_code connect()    = 0;
        virtual std::error_code reconnect(int delay_ms) = 0;
        virtual bool isConnected() const = 0;
        virtual void disconnect() = 0;
        virtual std::error_code readExactly(std::string& data, size_t n, int timeout_ms = 1000) = 0;
        virtual std::error_code readUntil(std::string& data, char delimiter, int timeout_ms = 1000) = 0;
        virtual std::error_code write(const std::string& data, int timeout_ms) = 0;
        
        virtual ~IBoardTransport() = default;
    };

}