namespace navigator::domain::value_objects {
    class Velocity {
        public:
            explicit Velocity(double vx, double vy, double vw);
            ~Velocity() = default;

            Velocity(const Velocity& other);
            Velocity(Velocity&& other) = delete;

            Velocity& operator=(const Velocity& other) = delete;
            Velocity& operator=(Velocity&& other) = delete;

            bool operator==(const Velocity& other) const; 

            double getVx() const;
            double getVy() const;
            double getVw() const;


        private:
            const double vx_;
            const double vy_;
            const double vw_;
    };
}