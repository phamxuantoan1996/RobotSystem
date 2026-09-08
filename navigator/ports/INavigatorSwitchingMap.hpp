#pragma once
#include <string>
#include <system_error>

namespace navigator::ports {
    class INavigatorSwitchingMap {
        public:
            virtual ~INavigatorSwitchingMap() = default;
            virtual std::error_code switchMap(std::string map_name) = 0;
    };
}