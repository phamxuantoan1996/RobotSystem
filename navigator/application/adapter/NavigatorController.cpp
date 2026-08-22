#include "NavigatorController.hpp"
#include "NavigatorEvent.hpp"
#include "INavigatorDriver.hpp"
#include "../common/application/EventBus.hpp"
#include "NavigatorState.hpp"
#include "station.hpp"
#include <iostream>
#include <memory>
#include <utility>
#include <variant>

namespace navigator::application::adapter {
    NavigatorController::NavigatorController(std::unique_ptr<navigator::ports::INavigatorDriver> driver)
    : driver_(std::move(driver))
    , navigatorEventBus_(std::make_unique<common::application::EventBus<navigator::domain::events::NavigatorEvent>>())
    , reconnectService_(navigator::application::services::NavigatorReconnectConfig{.maxRetries = 100, .retryIntervalMs = 3000})
    {
        
    }

    NavigatorController::~NavigatorController()
    {
        disconnect();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Lifecycle
    // ─────────────────────────────────────────────────────────────────────────────
    std::error_code NavigatorController::connect()
    {
        driver_->setNavigatorEventCallback([this](const domain::events::NavigatorEvent& e) {
            handleEvent(e);
        });
        return driver_->connect();
    }

    void NavigatorController::disconnect() { driver_->disconnect(); }
    bool NavigatorController::isConnected() const { return driver_->isConnected();}

    // ─────────────────────────────────────────────────────────────────────────────
    // Navigation commands
    // ─────────────────────────────────────────────────────────────────────────────
    std::error_code NavigatorController::goToStation(const domain::value_objects::Station& stationId)
    {
        return driver_->goToStation(stationId);
    }

    std::error_code NavigatorController::goToPoint(const domain::value_objects::Location& location, domain::entities::NavigatorBackMode back_mode, domain::entities::NavigatorCoordinate coordinate)
    {
        return driver_->goToPoint(location, back_mode, coordinate);
    }

    std::error_code NavigatorController::cancel()   { return driver_->cancelNavigation(); }
    std::error_code NavigatorController::pause()    { return driver_->pauseNavigation();  }
    std::error_code NavigatorController::resume()   { return driver_->resumeNavigation(); }

    // ─────────────────────────────────────────────────────────────────────────────
    // Control commands
    // ─────────────────────────────────────────────────────────────────────────────
    std::error_code NavigatorController::relocation(const domain::value_objects::Location& location)
    {
        return driver_->relocation(location);
    }

    std::error_code NavigatorController::confirmLocation()
    {
        return driver_->confirmRelocation();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // State
    // ─────────────────────────────────────────────────────────────────────────────
    domain::entities::NavigatorState NavigatorController::state() const
    {
        return driver_->getState();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // subscribeEvents — RobotSystem dùng để nhận tất cả NavEvent
    // ─────────────────────────────────────────────────────────────────────────────
    NavigatorController::HandlerId NavigatorController::subscribeEvents(NavigatorEventHandler handler)
    {
        return navigatorEventBus_->subscribe(std::move(handler));
    }

    void NavigatorController::unSubscribeEvents(NavigatorController::HandlerId id)
    {
        navigatorEventBus_->unsubscribe(id);
    }

    void NavigatorController::handleEvent(const domain::events::NavigatorEvent& event)
    {
        // 1. Dispatch typed callbacks
        std::visit([this](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, domain::events::NavigatorArrivedEvent>)
            {     
                std::cout << "Arrived.\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::NavigatorSetBlockEvent>)
            {
                std::cout << "Blocked.\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::NavigatorClearBlockEvent>)
            {
                std::cout << "Unblocked.\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::NavigatorSetEmergencyEvent>)
            {
                std::cout << "Emergency.\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::NavigatorClearEmergencyEvent>)
            {
                std::cout << "Clear emergency.\n";
            }
            else if constexpr (std::is_same_v<T, domain::events::NavigatorDisconnectEvent>) {
                // Fire lên EventBus trước
                // navigationEventBus_->publish(e);
                reconnectService_.startAsync(
                    // ConnectFn
                    [this]() -> std::error_code {
                        return driver_->connect();
                    },
                    // OnSuccess
                    [this]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        navigatorEventBus_->publish(domain::events::NavigatorReconnectEvent{});
                        std::cout << "Reconnect success\n";
                    },
                    // OnGiveUp — hết 5 lần retry
                    [this](int attempts) {
                        std::cerr << "[NavController] reconnect failed after " << attempts << " attempts\n";
                    }
                );
                return;
            }
            else if constexpr (std::is_same_v<T, domain::events::NavigatorReconnectEvent>) {
                // nav_event_bus_->publish(e);
                return;
            }
        }, event);

        // 2. Broadcast qua EventBus — RobotSystem nhận qua subscribeEvents()
        navigatorEventBus_->publish(event);
    }
}