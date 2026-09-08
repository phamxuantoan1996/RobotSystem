#pragma once

#include <system_error>
namespace navigator::ports {
    class INavigatorRecognition {
        public:
            virtual ~INavigatorRecognition() = default;
            virtual std::error_code setShelf(std::string shelf_name) = 0;
            virtual std::error_code clearShelf() = 0;
    };
}