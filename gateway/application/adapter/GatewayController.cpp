#include "GatewayController.hpp"
#include "GatewayEvent.hpp"
#include "application/EventBus.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

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
        gatewayEventBus_->publish(event);
    }

    void GatewayController::setGetRobotStatusCallback(gateway::ports::IRobotGateway::GatewayGetRobotStateCallback cb)
    {
        gatewayDriver_->setGatewayGetRobotCallback(std::move(cb));
    }
}