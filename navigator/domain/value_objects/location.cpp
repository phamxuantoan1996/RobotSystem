#include "location.hpp"
#include <stdexcept>

namespace navigator::domain::value_objects {
    Location::Location(double x,double y, double angle) : x_(x),y_(y),angle_(angle)
    {
        if(angle_ > 180 || angle < -180)
        {
            throw std::invalid_argument("Error: Value of angle is invalid!");
        }
    }

    Location::Location(const Location& other) : x_(other.x_),y_(other.y_),angle_(other.angle_)
    {
        
    }

    Location& Location::operator=(const Location& other)
    {
        return *this;
    }

    bool Location::operator==(const Location& other) const
    {
        return (x_ == other.x_) && (y_ == other.y_) && (angle_ == other.angle_);
    }

    double Location::getX() const
    {
        return x_;
    }

    double Location::getY() const
    {
        return y_;
    }

    double Location::getAngle() const
    {
        return angle_;
    }
}