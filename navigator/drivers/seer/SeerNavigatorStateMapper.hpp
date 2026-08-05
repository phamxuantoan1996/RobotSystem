#pragma once
#include "SeerNavigatorStatusAll1.hpp"
#include "NavigatorState.hpp"
#include "NavigatorEvent.hpp"
#include <vector>
#include <optional>
#include <string>
#include <cmath>

namespace navigator::drivers::seer {
    struct PrevSnapshot {
        navigator::domain::entities::NavigatorTaskState taskState = navigator::domain::entities::NavigatorTaskState::None;
        bool blocked = false;
        bool emergencyStop = false;
        bool isStopped = false;
        std::string targetId = "";
        navigator::domain::entities::NavigatorRelocationState relocState = navigator::domain::entities::NavigatorRelocationState::Unknown;
        std::unordered_map<std::string,std::string> errors;
        std::unordered_map<std::string,std::string> fatals;
    };

    class SeerNavigatorStateMapper {
        public:
            navigator::domain::entities::NavigatorState toNavigationState(const SeerStatusAll1& raw) const;

            std::vector<navigator::domain::events::NavigatorEvent> detectEvents(
                const PrevSnapshot& prev,
                const navigator::domain::entities::NavigatorState& next,
                std::string& activeTaskID) const;

            PrevSnapshot snapshotOf(const navigator::domain::entities::NavigatorState& state) const;

            void setExpectTargetId(std::string taskID, std::string targetID);

            void setGoToPointTarget(double x,double y, double theta, domain::entities::NavigatorPose pose0, domain::entities::NavigatorCoordinate coordinate);
            void clearGoToPointTarget() const;

        private:
            static constexpr double POSITION_TOLERANCE_MM = 10.0; // 1 cm
            static constexpr double ANGLE_TOLERANCE_RAD   = 0.0873; // 5 degree

            struct GoToPointTarget{
                double x = 0;
                double y = 0;
                double theta = 0;
                domain::entities::NavigatorPose pose0; // toa do luc gui len goToPoint. Chi dung cho che do self
                domain::entities::NavigatorCoordinate coordinate;
                bool valid = false;
            };

            mutable GoToPointTarget goToPointTarget_;

            // ── Angle helpers ─────────────────────────────────────────────────────────
            static double normalizeAngle(double angle);
            static double angleDiff(double a, double b);

            // ── Field mappers ─────────────────────────────────────────────────────────
            navigator::domain::entities::NavigatorTaskState mapTaskState(int seerCode) const;
            navigator::domain::entities::NavigatorRelocationState mapRelocState(int seerRelocStatus) const;
            navigator::domain::entities::NavigatorChargeState mapChargeState(const SeerStatusAll1& raw) const;

            mutable bool arrivedStamp_   = false;
            mutable std::string expectTaskId_ = "";
            mutable std::string expectTargetId_ = "";

            // ── Transition checkers ──
            std::optional<navigator::domain::events::NavigatorEvent> checkTaskArrived(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next,const std::string& activeTaskId) const;
            
            std::optional<navigator::domain::events::NavigatorEvent> checkTaskSuspended(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next, const std::string& activeTaskId) const;
            
            std::optional<navigator::domain::events::NavigatorEvent> checkTaskResumed(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next, const std::string& activeTaskId) const;
            
            std::optional<navigator::domain::events::NavigatorEvent> checkTaskCanceled(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next, const std::string& activeTaskId) const;

            std::optional<navigator::domain::events::NavigatorEvent> checkTaskFailed(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next, const std::string& activeTaskId) const;
            
            std::optional<navigator::domain::events::NavigatorEvent> checkBlocked(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next) const;
            
            std::optional<navigator::domain::events::NavigatorEvent> checkEmergency(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next) const;
            
            std::optional<navigator::domain::events::NavigatorEvent> checkRelocState(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next) const;
            
            std::optional<std::vector<navigator::domain::events::NavigatorEvent>> checkErrors(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next) const;
            
            std::optional<std::vector<navigator::domain::events::NavigatorEvent>> checkFatals(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next) const;

            std::optional<navigator::domain::events::NavigatorEvent> checkTaskStarted(const PrevSnapshot& prev, const navigator::domain::entities::NavigatorState& next) const;
    };
}