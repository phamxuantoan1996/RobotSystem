#include "SeerNavigatorStateMapper.hpp"
#include "NavigatorState.hpp"
#include <iostream>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
namespace navigator::drivers::seer {
    navigator::domain::entities::NavigatorState SeerNavigatorStateMapper::toNavigationState(const SeerStatusAll1& raw) const
    {
        domain::entities::NavigatorState s;

        // Pose
        s.pose.x          = raw.x;
        s.pose.y          = raw.y;
        s.pose.angle      = raw.angle;
        s.pose.confidence = raw.confidence;

        // Station
        s.currentStation = raw.current_station;
        s.lastStation    = raw.last_station;

        // Velocity (actual)
        s.vx        = raw.vx;
        s.vy        = raw.vy;
        s.w         = raw.w;
        s.isStopped = raw.is_stop;

        // Obstacle — blocked
        s.blocked.detected = raw.blocked;
        s.blocked.reason   = raw.blocked ? raw.block_reason : -1;
        s.blocked.x        = raw.block_x;
        s.blocked.y        = raw.block_y;

        // Obstacle — slowed
        s.slowed.detected  = raw.slowed;
        s.slowed.reason    = raw.slowed ? raw.slow_reason : -1;
        s.slowed.x         = raw.slow_x;
        s.slowed.y         = raw.slow_y;

        // Battery
        s.battery.level   = raw.battery_level;
        s.battery.voltage = raw.voltage;
        s.battery.charge  = mapChargeState(raw);

        // Task
        s.taskStatus = mapTaskState(raw.task_status);
        s.targetId   = raw.target_id;

        s.unfinished_path = raw.unfinished_path;
        s.finished_path = raw.finished_path;

        // System
        s.locStatus     = mapRelocState(raw.reloc_status);
        s.emergency = raw.emergency || raw.driver_emc;
        s.currentMap    = raw.current_map;

        s.errors = std::move(raw.errors);
        s.fatals = std::move(raw.fatals);

        s.state_raw = std::move(raw.state_raw);

        return s;
    }

    PrevSnapshot SeerNavigatorStateMapper::snapshotOf(const navigator::domain::entities::NavigatorState& state) const
    {
        return PrevSnapshot{
            .taskState    = state.taskStatus,
            .blocked       = state.blocked.detected,
            .emergencyStop = state.emergency,
            .isStopped = state.isStopped,
            .targetId      = state.targetId,
            .relocState = state.locStatus,
            .errors = state.errors,
            .fatals = state.fatals
        };
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Field mappers
    // ─────────────────────────────────────────────────────────────────────────────
    navigator::domain::entities::NavigatorTaskState SeerNavigatorStateMapper::mapTaskState(int seerCode) const
    {
        // SEER: 0=NONE 1=WAITING 2=RUNNING 3=SUSPENDED 4=COMPLETED 5=FAILED 6=CANCELED
        switch (seerCode) {
            case 0:  return navigator::domain::entities::NavigatorTaskState::None;
            case 1:  return navigator::domain::entities::NavigatorTaskState::Running;    // WAITING treated as Running
            case 2:  return navigator::domain::entities::NavigatorTaskState::Running;
            case 3:  return navigator::domain::entities::NavigatorTaskState::Suspended;
            case 4:  return navigator::domain::entities::NavigatorTaskState::Completed;
            case 5:  return navigator::domain::entities::NavigatorTaskState::Failed;
            case 6:  return navigator::domain::entities::NavigatorTaskState::Canceled;
            default: return navigator::domain::entities::NavigatorTaskState::None;
        }
    }

    navigator::domain::entities::NavigatorRelocationState SeerNavigatorStateMapper::mapRelocState(int seerRelocState) const
    {
        // SEER: 0=RELOC_INIT 1=RELOC_SUCCESS 2=RELOC_RELOCING
        switch (seerRelocState) {
            case 0:  return navigator::domain::entities::NavigatorRelocationState::Unknown;
            case 1:  return navigator::domain::entities::NavigatorRelocationState::Localised;
            case 2:  return navigator::domain::entities::NavigatorRelocationState::Relocalising;
            case 3: return navigator::domain::entities::NavigatorRelocationState::LocalisedConfirm;
            default: return navigator::domain::entities::NavigatorRelocationState::Unknown;
        }
    }

    navigator::domain::entities::NavigatorChargeState SeerNavigatorStateMapper::mapChargeState(const SeerStatusAll1& raw) const
    {
        if (!raw.charging)
            return navigator::domain::entities::NavigatorChargeState::Unplugged;
        if (raw.battery_level >= 0.99)
            return navigator::domain::entities::NavigatorChargeState::Full;
        return navigator::domain::entities::NavigatorChargeState::Charging;
    }

    void SeerNavigatorStateMapper::setExpectTargetId(std::string taskId, std::string targetId)
    {
        expectTaskId_   = taskId;
        expectTargetId_ = targetId;
    }

    void SeerNavigatorStateMapper::setGoToPointTarget(double x, double y, double theta,domain::entities::NavigatorPose pose0, domain::entities::NavigatorCoordinate coordinate)
    {
        goToPointTarget_ = GoToPointTarget{
            .x     = x,
            .y     = y,
            .theta = theta,
            .pose0 = pose0,
            .coordinate = coordinate,
            .valid = true
        };
        // Reset GoToStation target
        expectTaskId_   = "";
        expectTargetId_ = "";
    }

    void SeerNavigatorStateMapper::clearGoToPointTarget() const
    {
        goToPointTarget_ = GoToPointTarget{};
    }

    double SeerNavigatorStateMapper::normalizeAngle(double angle)
    {
        while (angle >  M_PI) angle -= 2.0 * M_PI;
        while (angle < -M_PI) angle += 2.0 * M_PI;
        return angle;
    }

    double SeerNavigatorStateMapper::angleDiff(double a, double b)
    {
        return std::abs(normalizeAngle(a - b));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // checkArrived
    //
    // GoToStation: check currentStation == expectedStation + unfinished_path empty
    // GoToPoint:   check pose distance <= 100mm AND angle diff <= 5°
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkTaskArrived(
        const PrevSnapshot&            prev,
        const navigator::domain::entities::NavigatorState& next,
        const std::string& activeTaskId) const
    {
        // std::cout << "detect\n";
        // Reset khi task mới bắt đầu Running
        if (next.taskStatus == navigator::domain::entities::NavigatorTaskState::Running)
            arrivedStamp_ = false;

        // Edge: non-Completed → Completed (normal flow: Running → Completed)
        if (next.taskStatus == navigator::domain::entities::NavigatorTaskState::Completed
        && prev.taskState != navigator::domain::entities::NavigatorTaskState::Completed)
            arrivedStamp_ = true;

        // Robot đã ở đích ngay khi nhận lệnh (không qua Running)
        // Detect: cả prev và next đều Completed nhưng là task mới
        if (next.taskStatus == navigator::domain::entities::NavigatorTaskState::Completed
        && prev.taskState == navigator::domain::entities::NavigatorTaskState::Completed
        && !expectTaskId_.empty()
        && activeTaskId == expectTaskId_
        && expectTargetId_ == next.currentStation)
            arrivedStamp_ = true;

        if (!arrivedStamp_ || !next.isStopped)
            return std::nullopt;

        // ── GoToPoint — check bằng tọa độ ────────────────────────────────────────
        if (goToPointTarget_.valid) 
        {
            if(goToPointTarget_.coordinate == domain::entities::NavigatorCoordinate::WORLD)
            {
                double dx   = next.pose.x - goToPointTarget_.x;
                double dy   = next.pose.y - goToPointTarget_.y;
                double dist = std::sqrt(dx*dx + dy*dy);
                double da   = angleDiff(next.pose.angle, goToPointTarget_.theta);

                // std::cout << "dx : " << dx << "\tdy : " << dy << "\tdist : " << dist << "\tda : " << (da *180)/3.14 << std::endl; 
                if (dist*1000 > POSITION_TOLERANCE_MM || da > ANGLE_TOLERANCE_RAD)
                    return std::nullopt;  // chưa đến đúng vị trí — tiếp tục chờ
                std::cout << "dist : " << dist << std::endl;
                std::cout << "da : " << da << std::endl;
                arrivedStamp_ = false;
                clearGoToPointTarget();
                return navigator::domain::events::NavigatorArrivedEvent{};
            }
            else if (goToPointTarget_.coordinate == domain::entities::NavigatorCoordinate::SELF) {
                double dx   = (next.pose.x - goToPointTarget_.pose0.x) - goToPointTarget_.x;
                double dy   = (next.pose.y - goToPointTarget_.pose0.y) - goToPointTarget_.y;
                double dist = std::sqrt(dx*dx + dy*dy);
                double da   = angleDiff(next.pose.angle - goToPointTarget_.pose0.angle, goToPointTarget_.theta);

                if (dist*1000 > POSITION_TOLERANCE_MM || da > ANGLE_TOLERANCE_RAD)
                    return std::nullopt;  // chưa đến đúng vị trí — tiếp tục chờ
                std::cout << "dist : " << dist << std::endl;
                std::cout << "da : " << da << std::endl;
                arrivedStamp_ = false;
                clearGoToPointTarget();
                return navigator::domain::events::NavigatorArrivedEvent{};
            }
            
        }

        // ── GoToStation — check bằng currentStation ───────────────────────────────
        // Ưu tiên expectTargetId_ (robot ở đích ngay), fallback về prev.targetId
        std::string expectedStation = !expectTargetId_.empty() ? expectTargetId_ : prev.targetId;
        if (next.currentStation != expectedStation || !next.unfinished_path.empty())
            return std::nullopt;
        arrivedStamp_   = false;
        expectTargetId_ = "";
        expectTaskId_   = "";
        return navigator::domain::events::NavigatorArrivedEvent{};
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // checkFailed — RUNNING → FAILED
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkTaskFailed(
        const PrevSnapshot& prev,
        const navigator::domain::entities::NavigatorState& next,
        const std::string& activeTaskId) const
    {
        if(prev.taskState != navigator::domain::entities::NavigatorTaskState::Failed && next.taskStatus == navigator::domain::entities::NavigatorTaskState::Failed)
        {
            return navigator::domain::events::NavigatorTaskSetFailedEvent{};
        }
        else if(prev.taskState == navigator::domain::entities::NavigatorTaskState::Failed && next.taskStatus != navigator::domain::entities::NavigatorTaskState::Failed)
        {
            return navigator::domain::events::NavigatorTaskClearFailedEvent{};
        }
        return std::nullopt;
    }

    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkTaskSuspended(
        const PrevSnapshot&       prev,
        const navigator::domain::entities::NavigatorState& next,
        const std::string&        activeTaskId) const
    {
        if(prev.taskState == navigator::domain::entities::NavigatorTaskState::Running && next.taskStatus == navigator::domain::entities::NavigatorTaskState::Suspended)
        {
            return domain::events::NavigatorTaskPausedEvent{};
        }

        return std::nullopt;
    }

    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkTaskResumed(
        const PrevSnapshot& prev,
        const navigator::domain::entities::NavigatorState& next,
        const std::string& activeTaskId) const
    {
        if(prev.taskState == domain::entities::NavigatorTaskState::Suspended && next.taskStatus == domain::entities::NavigatorTaskState::Running)
        {
            return domain::events::NavigatorTaskResumedEvent{};
        }
        return std::nullopt;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // checkCancel
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkTaskCanceled(
        const PrevSnapshot&       prev,
        const navigator::domain::entities::NavigatorState& next,
        const std::string&        activeTaskId) const
    {
        if(prev.taskState != domain::entities::NavigatorTaskState::Canceled && next.taskStatus == domain::entities::NavigatorTaskState::Canceled)
        {
            return domain::events::NavigatorTaskCanceledEvent{};
        }
        return std::nullopt;
    }

    // check running
    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkTaskStarted(
            const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next) const
    {
        if(prev.taskState != navigator::domain::entities::NavigatorTaskState::Running && next.taskStatus == navigator::domain::entities::NavigatorTaskState::Running)
        {
            return domain::events::NavigatorTaskStartedEvent{};
        }
        return  std::nullopt;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // checkBlocked — rising edge only (false → true)
    // Emits once when the robot first becomes blocked, not every poll tick.
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkBlocked(
        const PrevSnapshot& prev,
        const navigator::domain::entities::NavigatorState& next) const
    {
        if(!prev.blocked && next.blocked.detected)
            return navigator::domain::events::NavigatorSetBlockEvent{};
        else if (prev.blocked && !next.blocked.detected)
            return navigator::domain::events::NavigatorClearBlockEvent{};
        return std::nullopt;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // checkEmergency — emit on BOTH edges (pressed and released)
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkEmergency(
        const PrevSnapshot& prev,
        const navigator::domain::entities::NavigatorState& next) const
    {
        if (!prev.emergencyStop && next.emergency)
            return domain::events::NavigatorSetEmergencyEvent{};
        else if (prev.emergencyStop && !next.emergency) {
            return domain::events::NavigatorClearEmergencyEvent{};
        }
        return std::nullopt;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // checkRelocStatus
    // ─────────────────────────────────────────────────────────────────────────────
    std::optional<navigator::domain::events::NavigatorEvent> SeerNavigatorStateMapper::checkRelocState(
        const PrevSnapshot&       prev,
        const navigator::domain::entities::NavigatorState& next) const
    {
        if ((prev.relocState == navigator::domain::entities::NavigatorRelocationState::Relocalising && next.locStatus == navigator::domain::entities::NavigatorRelocationState::LocalisedConfirm)
            || (prev.relocState == navigator::domain::entities::NavigatorRelocationState::LocalisedConfirm && next.locStatus == navigator::domain::entities::NavigatorRelocationState::LocalisedConfirm))
        {
            return domain::events::NavigatorRelocationConfirmEvent{};
        }
        return std::nullopt;
    }

    // check errors
    std::optional<std::vector<navigator::domain::events::NavigatorEvent>> SeerNavigatorStateMapper::checkErrors(
        const PrevSnapshot& prev,const navigator::domain::entities::NavigatorState& next) const
    {
        std::unordered_map<std::string, int> diff_tracker;
        // duyet qua prev.errors
        for (const auto& [key, value] : prev.errors) {
            diff_tracker[key]++; 
        }

        // duyet qua next.errors
        for (const auto& [key, value] : next.errors) {
            if (diff_tracker.count(key)) { 
                diff_tracker.erase(key); // Xóa luôn khỏi tracker vì cả 2 unordered map deu co
            } else {
                diff_tracker[key] = -1;  // Nếu Map A chưa có, thì đây là key chỉ có ở B
            }
        }
        std::vector<domain::events::NavigatorEvent> events;
        // duyet qua diff_tracker de detect clear error va set error
        for(const auto& [key,value]:diff_tracker)
        {
            if(value < 0) // nam trong next.errors => set error event
            {
                events.push_back(domain::events::NavigatorSetErrorEvent{.code = key,.desc=next.errors.at(key)});
            }
            else { // nam trong prev.errors => clear error event
                events.push_back(domain::events::NavigatorClearErrorEvent{.code = key});
            }
        }
        if(!events.empty())
            return events;
        return std::nullopt;
    }

    // check fatals
    std::optional<std::vector<navigator::domain::events::NavigatorEvent>> SeerNavigatorStateMapper::checkFatals(
        const PrevSnapshot& prev,const navigator::domain::entities::NavigatorState& next) const
    {
        std::unordered_map<std::string, int> diff_tracker;
        // duyet qua prev.fatals
        for (const auto& [key, value] : prev.fatals) {
            diff_tracker[key]++; 
        }

        // duyet qua next.fatals
        for (const auto& [key, value] : next.fatals) {
            if (diff_tracker.count(key)) { 
                diff_tracker.erase(key); // Xóa luôn khỏi tracker vì cả 2 unordered map deu co
            } else {
                diff_tracker[key] = -1;  // Nếu Map A chưa có, thì đây là key chỉ có ở B
            }
        }
        std::vector<domain::events::NavigatorEvent> events;
        // duyet qua diff_tracker de detect clear error va set error
        for(const auto& [key,value]:diff_tracker)
        {
            if(value < 0) // nam trong next.fatals => set error event
            {
                events.push_back(domain::events::NavigatorSetFatalEvent{.code = key,.desc=next.fatals.at(key)});
            }
            else { // nam trong prev.fatals => clear fatals event
                events.push_back(domain::events::NavigatorClearFatalEvent{.code = key});
            }
        }
        if(!events.empty())
            return events;
        return std::nullopt;
    }

    std::vector<domain::events::NavigatorEvent> SeerNavigatorStateMapper::detectEvents(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next, std::string& activeTaskId) const
    {
        std::vector<domain::events::NavigatorEvent> events;

        if (auto e = checkEmergency(prev, next))
            events.push_back(std::move(*e));

        if (auto e = checkBlocked(prev, next))
            events.push_back(std::move(*e));

        if (auto e = checkRelocState(prev, next))
            events.push_back(std::move(*e));

        if (auto e = checkErrors(prev,next))
            events.insert(events.end(), e.value().begin(), e.value().end());
        
        if (auto e = checkFatals(prev,next))
            events.insert(events.end(), e.value().begin(), e.value().end());

        if (auto e = checkTaskStarted(prev, next))
            events.push_back(std::move(*e));
        
        if (!activeTaskId.empty()) {
            if (auto e = checkTaskArrived(prev, next, activeTaskId))
            {
                activeTaskId.clear();
                events.push_back(std::move(*e));
            }
            else if (auto e = checkTaskFailed(prev,next,activeTaskId))
            {
                events.push_back(std::move(*e));
            }
            else if (auto e = checkTaskSuspended(prev, next, activeTaskId))
                events.push_back(std::move(*e));
            else if (auto e = checkTaskResumed(prev, next, activeTaskId))
                events.push_back(std::move(*e));
            else if (auto e = checkTaskCanceled(prev, next, activeTaskId))
                events.push_back(std::move(*e));
        }

        return events;
    }
        
}