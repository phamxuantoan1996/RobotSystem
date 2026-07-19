#pragma once
#include <string>
namespace robot_system::domain::value_objects::navigation {
    class Station {
        public:
            explicit Station(const std::string& id);
            std::string getId() const;
        private:
            const std::string id_; // bien const chi gan duoc 1 lan
    };
}