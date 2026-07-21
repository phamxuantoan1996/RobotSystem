#pragma once
#include <string>
namespace navigator::domain::value_objects {
    class Station {
        public:
            explicit Station(const std::string& id);
            
            Station(const Station& other);
            Station(Station&& other) = delete;

            std::string getId() const;
        private:
            const std::string id_; // bien const chi gan duoc 1 lan
    };
}