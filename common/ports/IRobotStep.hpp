namespace common::ports {
    class IRobotStep {
        public:
            virtual ~IRobotStep() = default;
            virtual void excute() = 0;
            virtual void pause() = 0;
            virtual void resume() = 0;
            virtual void cancel() = 0;
    };
}