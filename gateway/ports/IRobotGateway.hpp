#pragma once
#include "../gateway/domain/entities/Network.hpp"
#include "../gateway/domain/events/GatewayEvent.hpp"
#include <functional>
#include <string>
namespace gateway::ports {
    class IRobotGateway {
        public:
            virtual ~IRobotGateway() = default;

            // sendStatus
            virtual gateway::domain::entities::NetworkResult sendStatus(const std::string& payload) = 0;
            
            // sendRequest
            virtual gateway::domain::entities::NetworkResult sendRequest(const std::string& payload) = 0;

            // sendReponse
            virtual gateway::domain::entities::NetworkResult sendResponse(const std::string& payload) = 0;

            // start
            virtual void start() = 0;

            // stop
            virtual void stop() = 0;

            using GatewayEventCallback = std::function<void(const gateway::domain::events::GatewayEvent& event)>;
            virtual void setGatewayEventCallback(GatewayEventCallback cb) = 0;
    };
}