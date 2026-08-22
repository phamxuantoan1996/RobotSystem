#pragma once
#include "../gateway/ports/IRobotGateway.hpp"
#include "../gateway/domain/entities/Network.hpp"
#include "../gateway/domain/entities/SignalType.hpp"
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
namespace gateway::drivers::rest {
    class RestGateway : public ports::IRobotGateway {
        public:
            RestGateway(const std::string& fleet_url, uint16_t listen_port);
            ~RestGateway();

            // sendStatus
            gateway::domain::entities::NetworkResult sendStatus(const std::string& payload) override;
            
            // sendRequest
            gateway::domain::entities::NetworkResult sendRequest(const std::string& payload) override;

            // sendReponse
            gateway::domain::entities::NetworkResult sendResponse(const std::string& payload) override;

            // start
            void start() override;

            // stop
            void stop() override;

            void setGatewayEventCallback(GatewayEventCallback cb) override;

            void setGatewayGetRobotCallback(GatewayGetRobotStateCallback cb) override;

        private:
            std::string fleetUrl_;
            uint16_t port_;
            std::atomic<bool> running_{false};
            std::thread drogonThread_;
            GatewayEventCallback eventCallback_;
            GatewayGetRobotStateCallback getRobotStatusCallback_;
            
            std::atomic<gateway::domain::entities::CollisionSignalType> signal_type{gateway::domain::entities::CollisionSignalType::Exception};
            std::atomic<gateway::domain::entities::TransferSignalType> transfer_type{gateway::domain::entities::TransferSignalType::Unknown};

            void drogonServerThread();
    };
}