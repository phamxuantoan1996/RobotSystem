#include "station.hpp"
#include <regex>
namespace navigator::domain::value_objects {
    Station::Station(const std::string& id) : id_(id)
    {
        std::regex pattern1("^LM\\d+$"); // check station co dung dinh dang LM
        std::regex pattern2("^CP\\d+$"); // check station co dung dinh dang CP
        if (!std::regex_match(id, pattern1) && !std::regex_match(id, pattern2)) 
        {
            throw std::invalid_argument("Error: Station name '" + id + "' is invalid!");
        }
    }

    Station::Station(const Station& other):id_(other.id_)
    {

    }

    bool Station::operator==(const Station& other) const
    {
        return this->id_ == other.id_;
    }

    std::string Station::getId() const
    {
        return id_;
    }
}