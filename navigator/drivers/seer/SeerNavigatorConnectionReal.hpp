#pragma once
#include "ISeerNavigationConnection.hpp"
#include "SeerNavigatorFrameCodec.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
namespace navigator::drivers::seer{

    class SeerNavigatorConnectionReal : public ports::ISeerNavigatorConnection
    {
        public:
            SeerNavigatorConnectionReal(const std::string& host, uint16_t port, uint32_t timeout);
            ~SeerNavigatorConnectionReal();

            SeerNavigatorConnectionReal(const SeerNavigatorConnectionReal& other) = delete;
            SeerNavigatorConnectionReal& operator=(const SeerNavigatorConnectionReal& other) = delete;



            std::error_code connect() override;
            void disconnect() override;
            bool isConnected() const override;
            std::optional<navigator::drivers::seer::SeerNavigatorFrame> sendRequest(const navigator::drivers::seer::SeerNavigatorFrame& req) override;
        private:
            int socketFd_;
            std::atomic<bool> connected_{false};

            std::string host_;
            uint16_t port_;
            uint32_t timeout_;

            std::mutex ioMutex_;
            mutable std::mutex connectedMutex_;
            bool sendRaw(const std::vector<uint8_t>& bytes);
            std::optional<navigator::drivers::seer::SeerNavigatorFrame> recvFrame();
            bool recvExact(uint8_t *buff, size_t n);

    };

}