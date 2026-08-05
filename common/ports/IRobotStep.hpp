namespace common::ports {
    class IRobotStep {
        public:
            virtual ~IRobotStep() = default;
            virtual RobotStepResult excute(RobotStepResult prevResult) = 0;
    };
}