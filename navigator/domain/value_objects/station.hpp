#pragma once
#include <string>
namespace navigator::domain::value_objects {
    class Station {
        public:
            explicit Station(const std::string& id);
            std::string getId() const;
        private:
            const std::string id_; // bien const chi gan duoc 1 lan
    };
}