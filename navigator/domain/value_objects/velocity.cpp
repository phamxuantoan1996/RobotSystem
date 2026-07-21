#include "velocity.hpp"

namespace navigator::domain::value_objects {
    Velocity::Velocity(double vx, double vy, double vw) : vx_(vx), vy_(vy), vw_(vw)
    {

    }

    Velocity::Velocity(const Velocity& other) : vx_(other.vx_), vy_(other.vy_), vw_(other.vw_)
    {

    }

    bool Velocity::operator==(const Velocity& other) const
    {
        return (vw_ == other.vw_) && (vx_ == other.vx_) && (vy_ && other.vy_);
    }

    double Velocity::getVx() const
    {
        return vx_;
    }
    double Velocity::getVy() const
    {
        return vy_;
    }
    double Velocity::getVw() const
    {
        return vw_;
    }
}