#pragma once
#include "../common/ports/ISocketTcpClient.hpp"
#include "SeerNavigatorFrameCodec.hpp"
#include <optional>

namespace navigator::ports {

    class ISeerNavigatorConnection : public  common::ports::ISocketTcpClient {
        public:
            virtual ~ISeerNavigatorConnection() = default;
            virtual std::optional<navigator::drivers::seer::SeerNavigatorFrame> sendRequest(const navigator::drivers::seer::SeerNavigatorFrame& req) = 0;
    };

}