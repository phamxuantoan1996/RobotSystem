#pragma once
#include <string>
namespace navigator::domain::value_objects {
    class Station {
        public:
            explicit Station(const std::string& id);
            
            Station(const Station& other);
            Station(Station&& other) = delete;

            Station& operator=(const Station& other) = delete;
            Station& operator=(Station&& other) = delete;

            bool operator==(const Station& other) const;

            std::string getId() const;
        private:
            const std::string id_; // bien const chi gan duoc 1 lan
    };
}