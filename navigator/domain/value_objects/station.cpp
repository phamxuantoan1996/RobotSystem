#include "station.hpp"
#include <regex>
namespace robot_system::domain::value_objects::navigation {
    Station::Station(const std::string& id) : id_(id)
    {
        std::regex pattern1("^LM\\d+$"); // check station co dung dinh dang LM
        std::regex pattern2("^CP\\d+$"); // check station co dung dinh dang CP
        if (!std::regex_match(id, pattern1) && !std::regex_match(id, pattern2)) 
        {
            throw std::invalid_argument("Error: Station name '" + id + "' is invalid!");
        }
    }
    std::string Station::getId() const
    {
        return id_;
    }
}