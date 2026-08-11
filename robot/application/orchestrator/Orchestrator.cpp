#include "Orchestrator.hpp"
#include "RobotTask.hpp"
#include "ports/IRobotStep.hpp"
#include <iostream>
#include <mutex>
#include <thread>

namespace robot::application {
    Orchestrator::~Orchestrator()
    {
        running_ = false;
        cvEnqueue_.notify_all();
        if(workerThread_.joinable())
        {
            workerThread_.join();
        }
    }

    void Orchestrator::start() {
        if (running_) return; // Nếu đang chạy rồi thì bỏ qua
        
        running_ = true;
        workerThread_ = std::thread(&Orchestrator::workerTask, this);
    }

    std::string Orchestrator::getMissionId() const
    {
        std::lock_guard<std::mutex> lk(mutexState_);
        return missionIdCurrent_;
    }
    int Orchestrator::getStepIndex() const
    {
        std::lock_guard<std::mutex> lk(mutexState_);
        return stepIndex_;
    }
    robot::domain::entities::RobotTaskStatus Orchestrator::getTaskStatus() const
    {
        std::lock_guard<std::mutex> lk(mutexState_);
        return taskStatus_;
    }

    void Orchestrator::cancel()
    {
        cancelRequest = true;
    }

    bool Orchestrator::enqueueMission(robot::domain::entities::RobotTask&& robot_task)
    {
        if (!running_) {
            return false;
        }
        taskQueue_.enqueue(std::move(robot_task));
        cvEnqueue_.notify_one();
        std::cout << "orchestrator queue notify\n";
        return true;
    }

    void Orchestrator::workerTask()
    {
        while (running_) {
            std::optional<robot::domain::entities::RobotTask> task;
            {
                std::lock_guard<std::mutex> lk(mutexEnqueue_);
                task = taskQueue_.dequeue();
            }
            {
                if(!task.has_value())
                {
                    std::unique_lock<std::mutex> lock(mutexEnqueue_);
                    cvEnqueue_.wait(lock,[&]{
                        return !taskQueue_.isEmpty() || !running_;
                    });
                    task = taskQueue_.dequeue();
                }
            }

            if(task.has_value())
            {
                {
                    missionIdCurrent_ = task->mission_id;
                    stepIndex_ = 0;
                    taskStatus_ = robot::domain::entities::RobotTaskStatus::Running;
                    if(onMissionRunning_)
                    {
                        onMissionRunning_(missionIdCurrent_);
                    }
                }
                common::ports::RobotStepResult prevResultStep = common::ports::UnknowStepResult{};
                for(auto const& step : task->steps)
                {
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        stepIndex_ = step->getActionIndex();
                    }
                    auto result = step->excute(prevResultStep);
                    std::visit([this](const auto& res) {
                        using T = std::decay_t<decltype(res)>;
                        if constexpr (std::is_same_v<T, common::ports::GotoStationStepResult>)
                        {     
                            {
                                std::lock_guard<std::mutex> lk(mutexState_);
                                if(res.result == common::ports::GotoStationStepResult::Result::Canceled)
                                {
                                    taskStatus_ = robot::domain::entities::RobotTaskStatus::Canceled;
                                }
                                else if(res.result == common::ports::GotoStationStepResult::Result::Failed)
                                {
                                    taskStatus_ = robot::domain::entities::RobotTaskStatus::Error;
                                }
                            }
                        }
                    },result);
                    {
                        std::lock_guard<std::mutex> lk(mutexState_);
                        if(taskStatus_ != domain::entities::RobotTaskStatus::Running)
                        {
                            break;
                        }
                    }
                    prevResultStep = result;
                }
                // check task status goi callback cua robot controller
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    if(taskStatus_ == domain::entities::RobotTaskStatus::Canceled)
                    {
                        if(onMissionCanceled_)
                        {
                            onMissionCanceled_(missionIdCurrent_);
                        }
                    }
                    else if(taskStatus_ == domain::entities::RobotTaskStatus::Error)
                    {
                        if(onMissionError_)
                        {
                            onMissionError_(missionIdCurrent_);
                        }
                    }
                    else if(taskStatus_ == domain::entities::RobotTaskStatus::Running)
                    {
                        taskStatus_ = domain::entities::RobotTaskStatus::Completed;
                        if(onMissionCompleted_)
                        {
                            onMissionCompleted_(missionIdCurrent_);
                        }
                    }
                    stepIndex_ = -1;
                }
            }
            else {
                if (!running_) {
                    break;
                }
            }     
        }
    }

    void Orchestrator::setMissionRunningCallback(MissionRunningCallback cb)
    {
        onMissionRunning_ = std::move(cb);
    }
    void Orchestrator::setMissionCompletedCallback(MissionCompletedCallback cb)
    {
        onMissionCompleted_ = std::move(cb);
    }
    void Orchestrator::setMissionCanceledCallback(MissionCanceledCallback cb)
    {
        onMissionCanceled_ = std::move(cb);
    }
    void Orchestrator::setMissionErrorCallback(MissionErrorCallback cb)
    {
        onMissionError_ = std::move(cb);
    }
}