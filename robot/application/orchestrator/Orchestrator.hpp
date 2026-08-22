#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include "../robot/domain/entities/RobotTask.hpp"
#include "../robot/domain/value_objects/RobotTaskQueue.hpp"
#include "ports/IRobotStep.hpp"
namespace robot::application {
    class Orchestrator {
        public:
            public:
                explicit Orchestrator(){}
                ~Orchestrator();

                std::string getMissionId() const;
                int getStepIndex() const;
                robot::domain::entities::RobotTaskStatus getTaskStatus() const;

                void cancel();
                void pause();
                void resume();
                
                bool enqueueMission(robot::domain::entities::RobotTask&& robot_task);

                void start();

                using MissionRunningCallback = std::function<void(const std::string)>;
                using MissionCompletedCallback = std::function<void(const std::string)>;
                using MissionCanceledCallback = std::function<void(const std::string)>;
                using MissionErrorCallback = std::function<void(const std::string)>;

                void setMissionRunningCallback(MissionRunningCallback cb);
                void setMissionCompletedCallback(MissionCompletedCallback cb);
                void setMissionCanceledCallback(MissionCanceledCallback cb);
                void setMissionErrorCallback(MissionErrorCallback cb);
                
            private:
                std::thread workerThread_;
                std::atomic<bool> running_ = {false};
                std::atomic<bool> cancelRequest_ = {false};
                std::atomic<bool> pauseRequest_ = {false};

                std::string missionIdCurrent_ = "";
                robot::domain::entities::RobotTaskStatus taskStatus_ = robot::domain::entities::RobotTaskStatus::Unknown;
                std::atomic<int> stepIndex_{1};
                common::ports::IRobotStep* currentRunningStep_{nullptr}; 
                mutable std::mutex mutexState_;

                robot::domain::value_objects::RobotTaskQueue taskQueue_;
                std::condition_variable cvEnqueue_;
                
                std::mutex mutexEnqueue_;

                MissionRunningCallback onMissionRunning_;
                MissionCompletedCallback onMissionCompleted_;
                MissionCanceledCallback onMissionCanceled_;
                MissionErrorCallback onMissionError_;

                void workerTask();
                
    };
}