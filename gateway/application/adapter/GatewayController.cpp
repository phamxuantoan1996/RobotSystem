#include "GatewayController.hpp"
#include "GatewayEvent.hpp"
#include "application/EventBus.hpp"
#include <iostream>
#include <memory>
#include <string>

namespace gateway::application::adapter {
    GatewayController::GatewayController(std::unique_ptr<gateway::ports::IRobotGateway> gatewayDriver) 
    : gatewayDriver_(std::move(gatewayDriver)),
    gatewayEventBus_(std::make_unique<common::application::EventBus<gateway::domain::events::GatewayEvent>>())
    {

    }
    GatewayController::~GatewayController()
    {
        gatewayDriver_->stop();
    }
    gateway::domain::entities::NetworkResult GatewayController::sendStatus(const std::string& payload)
    {
        return gatewayDriver_->sendStatus(payload);
    }
    gateway::domain::entities::NetworkResult GatewayController::sendRequest(const std::string& payload)
    {
        return gatewayDriver_->sendRequest(payload);
    }
    gateway::domain::entities::NetworkResult GatewayController::sendResponse(const std::string& payload)
    {
        return gatewayDriver_->sendResponse(payload);
    }

    void GatewayController::start()
    {
        gatewayDriver_->setGatewayEventCallback([this](const domain::events::GatewayEvent& e) {
            handleEvent(e);
        });
        gatewayDriver_->start();
    }
    void GatewayController::stop()
    {
        gatewayDriver_->stop();
    }

    GatewayController::HandlerId GatewayController::subcribeEvents(GatewayEventHandler handler)
    {
        return gatewayEventBus_->subscribe(std::move(handler));
    }
    void GatewayController::unSubcribeEvents(GatewayController::HandlerId id)
    {
        gatewayEventBus_->unsubscribe(id);
    }

    void GatewayController::handleEvent(const gateway::domain::events::GatewayEvent& event)
    {
        std::visit([](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, domain::events::SignalCancelEvent>)
            {     
                std::cout << "signal cancel\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::SignalPauseEvent>) {
                std::cout << "signal pause\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::SignalResumeEvent>) {
                std::cout << "signal resume\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::SignalClearErrorEvent>) {
                std::cout << "signal clear error\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::SignalSwitchModeEvent>) {
                std::cout << "switch mode\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::SignalCollisionEvent>) {
                std::cout << "signal collision\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::SignalTransferEvent>) {
                std::cout << "signal transfer\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::MissionDispatchEvent>) {
                std::cout << "mission dispatch\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::ControlManualEvent>) {
                std::cout << "control manual\n";
            }
        },event);
        gatewayEventBus_->publish(event);
    }
}