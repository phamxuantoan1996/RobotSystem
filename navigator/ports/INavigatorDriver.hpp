#pragma once
#include <system_error>
#include <functional>
#include "NavigatorState.hpp"
#include "NavigatorEvent.hpp"
#include "location.hpp"
#include "station.hpp"

namespace navigator::ports {
    class INavigatorDriver {
        public:
            virtual ~INavigatorDriver() = default;

            virtual std::error_code connect() = 0;
            virtual void disconnect() = 0;
            virtual bool isConnected() const = 0;

            virtual std::error_code goToStation(const navigator::domain::value_objects::Station& station) = 0;
            virtual std::error_code goToPoint(const navigator::domain::value_objects::Location& location,domain::entities::NavigatorBackMode backMode, domain::entities::NavigatorCoordinate navigatorCoordinate) = 0;

            virtual std::error_code cancelNavigation() = 0;
            virtual std::error_code pauseNavigation() = 0;
            virtual std::error_code resumeNavigation() = 0;

            virtual std::error_code relocation(const navigator::domain::value_objects::Location& location) = 0;
            virtual std::error_code confirmRelocation() = 0;

            virtual navigator::domain::entities::NavigatorState getState() const = 0;

            using NavigatorEventCallback = std::function<void(const navigator::domain::events::NavigatorEvent& event)>;
            virtual void setNavigatorEventCallback(NavigatorEventCallback cb) = 0;
            
    };
}