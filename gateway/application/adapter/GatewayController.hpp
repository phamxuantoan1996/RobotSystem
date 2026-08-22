#pragma once
#include "../gateway/ports/IRobotGateway.hpp"
#include "../gateway/domain/entities/Network.hpp"
#include "../gateway/domain/events/GatewayEvent.hpp"
#include "../common/application/EventBus.hpp"
#include "../common/ports/IEventBus.hpp"
#include <memory>
#include <string>

namespace gateway::application::adapter {
    class GatewayController {
        public:
            GatewayController(std::unique_ptr<gateway::ports::IRobotGateway> gatewayDriver);
            ~GatewayController();

            GatewayController(const GatewayController& other) = delete;
            GatewayController& operator=(const GatewayController& other) = delete;

            GatewayController(GatewayController&& other) = delete;
            GatewayController& operator=(GatewayController&& other) = delete;

            gateway::domain::entities::NetworkResult sendRequest(const std::string& payload);
            gateway::domain::entities::NetworkResult sendResponse(const std::string& payload);
            gateway::domain::entities::NetworkResult sendStatus(const std::string& payload);

            void start();
            void stop();

            void setGetRobotStatusCallback(gateway::ports::IRobotGateway::GatewayGetRobotStateCallback cb);

            using GatewayEventHandler = std::function<void(const gateway::domain::events::GatewayEvent&)>;
            using HandlerId = typename common::ports::IEventBus<gateway::domain::events::GatewayEvent>::HandlerID;
            HandlerId subcribeEvents(GatewayEventHandler handler);
            void unSubcribeEvents(HandlerId id);
        private:
            std::unique_ptr<gateway::ports::IRobotGateway> gatewayDriver_;
            std::unique_ptr<common::application::EventBus<gateway::domain::events::GatewayEvent>> gatewayEventBus_;
            void handleEvent(const gateway::domain::events::GatewayEvent& event);

    };
}