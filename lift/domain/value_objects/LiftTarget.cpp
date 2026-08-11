#include "LiftTarget.hpp"
#include <stdexcept>

namespace lift::domain::value_objects {
    
    LiftTarget::LiftTarget(int targetValue) : target(targetValue) 
    {
        // Thực hiện Validate ngay khi object được sinh ra
        if (targetValue < MIN_TARGET || targetValue > MAX_TARGET) {
            throw std::invalid_argument(
                "Domain Exception: Gia tri target phai nam trong khoang tu " + 
                std::to_string(MIN_TARGET) + " den " + std::to_string(MAX_TARGET) + 
                ". Gia tri ban truyen vao la: " + std::to_string(targetValue)
            );
        }
    }

    int LiftTarget::getTarget() const {
        return target;
    }

    bool LiftTarget::operator==(const LiftTarget& other) const {
                return target == other.target;
            }

    bool LiftTarget::operator!=(const LiftTarget& other) const {
        return !(*this == other);
    }
}