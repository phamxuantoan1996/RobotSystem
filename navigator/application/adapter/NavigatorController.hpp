#include "INavigatorDriver.hpp"
#include "../common/application/EventBus.hpp"
#include "../common/ports/IEventBus.hpp"
#include "NavigatorEvent.hpp"
#include "NavigatorReconnectService.hpp"
#include "NavigatorState.hpp"
#include "station.hpp"
// #include "navigation/application/services/ReconnectService.h"

#include <functional>
#include <memory>
#include <system_error>

namespace navigator::application::adapter {
    class NavigatorController {
        public:
            explicit NavigatorController(std::unique_ptr<navigator::ports::INavigatorDriver> driver);
            ~NavigatorController();

            NavigatorController(const NavigatorController& other) = delete;
            NavigatorController& operator=(const NavigatorController& other) = delete;

            NavigatorController(NavigatorController&& other) = delete;
            NavigatorController& operator=(NavigatorController&& other) = delete;

            std::error_code connect();
            void disconnect();
            bool isConnected() const;

            std::error_code goToStation(const domain::value_objects::Station& station);
            std::error_code goToPoint(const domain::value_objects::Location& location, domain::entities::NavigatorBackMode back_mode, domain::entities::NavigatorCoordinate coordinate);

            std::error_code cancel();
            std::error_code pause();
            std::error_code resume();

            std::error_code relocation(const domain::value_objects::Location& location);
            std::error_code confirmLocation();

            domain::entities::NavigatorState state() const;

            using NavigatorEventHandler = std::function<void(const navigator::domain::events::NavigatorEvent&)>;
            using HandlerId = typename common::ports::IEventBus<navigator::domain::events::NavigatorEvent>::HandlerID;
            HandlerId subcribeEvents(NavigatorEventHandler handler);
            void unSubcribeEvents(HandlerId id);


        private:
            void handleEvent(const navigator::domain::events::NavigatorEvent& event);
            std::unique_ptr<navigator::ports::INavigatorDriver> driver_;
            std::unique_ptr<common::application::EventBus<navigator::domain::events::NavigatorEvent>> navigatorEventBus_;

            navigator::application::services::NavigatorReconnectService reconnectService_;
    };
}