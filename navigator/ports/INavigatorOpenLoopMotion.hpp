#pragma once
#include "../navigator/domain/value_objects/velocity.hpp"
#include <cstdint>
#include <system_error>

namespace navigator::ports {
    class INavigatorOpenLoopMotion {
        public:
            virtual ~INavigatorOpenLoopMotion() = default;
            virtual std::error_code openLoopMotion(navigator::domain::value_objects::Velocity v,uint32_t duration) = 0;
            virtual std::error_code stopOpenLoopMotion() = 0;
    };
}