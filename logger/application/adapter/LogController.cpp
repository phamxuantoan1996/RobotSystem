#include "../logger/application/adapter/LogController.hpp"
#include "../lift/domain/events/LiftEvent.hpp"
#include "../navigator/domain/events/NavigatorEvent.hpp"
#include "../robot/domain/events/RobotEvent.hpp"
#include <string>
#include <type_traits>
#include <variant>
#include <jsoncpp/json/json.h>

namespace logger::application::adapter {
    LogController::LogController(
                std::unique_ptr<logger::ports::ILogWriter> writer,
                std::shared_ptr<robot::application::RobotController> robotController,
                std::shared_ptr<board::application::adapter::BoardController> boardController,
                std::shared_ptr<navigator::application::adapter::NavigatorController> navigatorController,
                std::shared_ptr<lift::application::adapter::LiftController> liftController)
        : writer_(std::move(writer))
        {
            robotController->subscribeEvents([this](const robot::domain::events::RobotEvent& event){
                std::visit([this](const auto& e){
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, robot::domain::events::MissionAcceptedEvent>)
                    {
                        log(LogLevel::Info, "Robot", "Mission raw : " + e.mission_raw);
                    }
                    else if constexpr (std::is_same_v<T, robot::domain::events::MissionRejectedEvent>)
                    {
                        log(LogLevel::Info, "Robot", "Mission raw : " + e.mission_raw);
                    }
                    else if constexpr (std::is_same_v<T, robot::domain::events::MissionRunningEvent>)
                    {
                        log(LogLevel::Info, "Robot", "Mission running.");
                    }
                    else if constexpr (std::is_same_v<T, robot::domain::events::MissionErrorEvent>)
                    {
                        log(LogLevel::Info, "Robot", "Mission error");
                    }
                    else if constexpr (std::is_same_v<T, robot::domain::events::MissionCompletedEvent>)
                    {
                        log(LogLevel::Info, "Robot", "Mission completed");
                    }
                    else if constexpr (std::is_same_v<T, robot::domain::events::MissionCanceledEvent>)
                    {
                        log(LogLevel::Info, "Robot", "Mission canceled");
                    }
                }, event);
            });

            liftController->subscribeEvents([this](const lift::domain::events::LiftEvent& event){
                std::visit([this](const auto& e){
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusSetErrorEvent>)
                    {
                        log(LogLevel::Error, "Lift", "Error code : " + std::to_string(e.error_code));
                    }
                    else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusClearErrorEvent>)
                    {
                        log(LogLevel::Info, "Lift", "Clear error code : " + std::to_string(e.error_code));
                    }
                    else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskRunningEvent>)
                    {
                        log(LogLevel::Info, "Lift", "Running");
                    }
                    else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskPausedEvent>)
                    {
                        log(LogLevel::Info, "Lift", "Paused");
                    }
                    else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCanceledEvent>)
                    {
                        log(LogLevel::Info, "Lift", "Canceled");
                    }
                    else if constexpr (std::is_same_v<T, lift::domain::events::LiftTaskCompletedEvent>)
                    {
                        log(LogLevel::Info, "Lift", "Completed");
                    }
                    else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusSetEmergency>)
                    {
                        log(LogLevel::Info, "Lift", "Set emergency");
                    }
                    else if constexpr (std::is_same_v<T, lift::domain::events::LiftStatusClearEmergency>)
                    {
                        log(LogLevel::Info, "Lift", "Clear emergency");
                    }
                    
                }, event);
            });

            navigatorController->subscribeEvents([this](const navigator::domain::events::NavigatorEvent& event){
                std::visit([this](const auto& e){
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorDisconnectEvent>) {
                        log(LogLevel::Error, "Navigator", "Disconnected");
                    }
                    else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorReconnectEvent>) {
                        log(LogLevel::Error, "Navigator", "Reconnected");
                    }
                    else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorArrivedEvent>) {
                        log(LogLevel::Info, "Navigator", "Arrived");
                    }
                    else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetEmergencyEvent>) {
                        log(LogLevel::Error, "Navigator", "Set Emergency");
                    }
                    else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorClearEmergencyEvent>) {
                        log(LogLevel::Error, "Navigator", "Clear Emergency");
                    }
                    else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetErrorEvent>) {
                        log(LogLevel::Error, "Navigator", "Error code : " + e.code + ". Desc : " + e.desc);
                    }
                    else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorSetFatalEvent>) {
                        log(LogLevel::Error, "Navigator", "Fatal code : " + e.code + ". Desc : " + e.desc);
                    }
                    else if constexpr (std::is_same_v<T, navigator::domain::events::NavigatorTaskSetFailedEvent>) {
                        log(LogLevel::Error, "Navigator", "Task failed");
                    }
                }, event);
            });
            
            boardController->subscribeEvents([this](const board::domain::events::BoardEvent& event){
                std::visit([this](const auto& e){
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, board::domain::events::BoardDisconnectedEvent>) {
                        log(LogLevel::Error, "Board", "Disconnected");
                    }
                    else if constexpr (std::is_same_v<T, board::domain::events::BoardReconnectedEvent>) {
                         log(LogLevel::Info, "Board", "Reconnected");
                    }
                }, event);
            });
        }

    void LogController::log(LogLevel level,
             const std::string& subsystem,
             const std::string& message)
    {
        writer_->write(logger::domain::entities::LogEntry{
            .timestamp = std::chrono::system_clock::now(),
            .level     = level,
            .subsystem = subsystem,
            .message   = message
        });
    }
}