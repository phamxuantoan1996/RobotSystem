namespace navigator::domain::value_objects {
    class Location {
        public:
            explicit Location(double x,double y, double angle);
            ~Location() = default;

            Location(const Location& other);
            Location(Location&& other) = delete;

            Location& operator=(const Location& other);
            Location& operator=(Location&& other) = delete;

            bool operator==(const Location& other);
            
            double getX();
            double getY();
            double getAngle();

        private:
            const double x_;
            const double y_;
            const double angle_;
    };
}