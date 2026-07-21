#include <iostream>
#include "navigator/domain/value_objects/station.hpp"

int main(int argc, char *argv[])
{
    try 
    {
        navigator::domain::value_objects::Station station1("LM123");
        std::cout << "Station Id : " << station1.getId() << std::endl;

        navigator::domain::value_objects::Station station2(station1);
        std::cout << "Station Id : " << station2.getId() << std::endl;

        navigator::domain::value_objects::Station station3("LM12");

        if(station1 == station3)
        {
            std::cout << "match\n";
        }
        else {
            std::cout << "don't match\n";
        }
    } 
    catch (const std::invalid_argument& e) 
    {
        // Nếu constructor quăng ra lỗi, code sẽ nhảy ngay lập tức vào đây
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}