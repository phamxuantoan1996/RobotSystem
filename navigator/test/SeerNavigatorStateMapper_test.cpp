#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include "NavigatorState.hpp"
#include "NavigatorEvent.hpp"
#include "SeerNavigatorStateMapper.hpp"

namespace navigator::drivers::seer {
    class SeerNavigatorStateMapper_test : public ::testing::Test
    {
        protected:
            SeerNavigatorStateMapper mapper_;

            SeerStatusAll1 makeDefaultRaw()
            {
                SeerStatusAll1 raw;

                // Pose
                raw.x = 1.0;
                raw.y = 2.0;
                raw.angle = 0.5;
                raw.confidence = 0.9;

                // Station
                raw.current_station = "LM1";
                raw.last_station = "LM2";
                raw.target_id = "LM3";
                raw.unfinished_path = {"LM4", "LM5"};
                raw.finished_path = {"LM6"};

                // Velocity
                raw.vx = 0.1;
                raw.vy = 0.2;
                raw.w = 0.3;
                raw.is_stop = false;

                // Blocked
                raw.blocked = false;
                raw.block_reason = 2;
                raw.block_x = 5.0;
                raw.block_y = 6.0;

                // Slowed
                raw.slowed = false;
                raw.slow_reason = 2;
                raw.slow_x = 7.0;
                raw.slow_y = 8.0;

                // Battery
                raw.battery_level = 0.8;
                raw.voltage = 24.0;
                raw.current = 2.0;
                raw.charging = false;

                // Task
                raw.task_status = 0;

                // Relocation
                raw.reloc_status = 0;

                // Emergency
                raw.emergency = false;
                raw.driver_emc = false;

                // Errors / Fatals
                raw.errors.clear();
                raw.fatals.clear();

                // Map
                raw.current_map = "map_001.smap";

                // Raw state
                raw.state_raw = "{\"key\":\"value\"}";

                return raw;
            }

            PrevSnapshot makeDefaultPrevSnapshot()
            {
                return PrevSnapshot{};
            }

            domain::entities::NavigatorState makeDefaultNavigatorState()
            {
                return domain::entities::NavigatorState{};
            }
            bool hasArrivedEvent(const std::vector<navigator::domain::events::NavigatorEvent>& events)
            {
                return std::any_of(
                    events.begin(),
                    events.end(),
                    [](const auto& event)
                    {
                        return std::holds_alternative<
                            navigator::domain::events::NavigatorArrivedEvent>(event);
                    });
            }

            std::vector<navigator::domain::events::NavigatorEvent> detectEvents(
                const PrevSnapshot& prev,
                const navigator::domain::entities::NavigatorState& next,
                const std::string& activeTaskID = "")
            {
                std::string taskID = activeTaskID;
                return mapper_.detectEvents(
                    prev,
                    next,
                    taskID);
            }
            
    };
    TEST_F(SeerNavigatorStateMapper_test, ToNavigationState_MapsRawStateCorrectly)
    {
        auto raw = makeDefaultRaw();
        auto state = mapper_.toNavigationState(raw);

        // pose
        EXPECT_DOUBLE_EQ(state.pose.x, raw.x);
        EXPECT_DOUBLE_EQ(state.pose.y, raw.y);
        EXPECT_DOUBLE_EQ(state.pose.angle, raw.angle);
        EXPECT_DOUBLE_EQ(state.pose.confidence, raw.confidence);

        // Station
        EXPECT_EQ(state.currentStation, raw.current_station);
        EXPECT_EQ(state.lastStation, raw.last_station);
        EXPECT_EQ(state.targetId, raw.target_id);
        EXPECT_EQ(state.unfinished_path, raw.unfinished_path);
        EXPECT_EQ(state.finished_path, raw.finished_path);

        // Velocity
        EXPECT_DOUBLE_EQ(state.vx, raw.vx);
        EXPECT_DOUBLE_EQ(state.vy, raw.vy);
        EXPECT_DOUBLE_EQ(state.w, raw.w);
        EXPECT_EQ(state.isStopped, raw.is_stop);

        // Blocked
        EXPECT_EQ(state.blocked.detected, raw.blocked);
        EXPECT_DOUBLE_EQ(state.blocked.x, raw.block_x);
        EXPECT_DOUBLE_EQ(state.blocked.y, raw.block_y);
        raw.blocked = true;
        state = mapper_.toNavigationState(raw);
        EXPECT_EQ(state.blocked.reason, raw.block_reason);
        raw.blocked = false;
        state = mapper_.toNavigationState(raw);
        EXPECT_EQ(state.blocked.reason, -1);
        

        // Slowed
        EXPECT_EQ(state.slowed.detected, raw.slowed);
        EXPECT_DOUBLE_EQ(state.slowed.x, raw.slow_x);
        EXPECT_DOUBLE_EQ(state.slowed.y, raw.slow_y);
        raw.slowed = false;
        state = mapper_.toNavigationState(raw);
        EXPECT_EQ(state.slowed.reason, -1);
        raw.slowed = true;
        state = mapper_.toNavigationState(raw);
        EXPECT_EQ(state.slowed.reason, raw.slow_reason);
        

        // Battery
        EXPECT_DOUBLE_EQ(state.battery.level, raw.battery_level);
        EXPECT_DOUBLE_EQ(state.battery.voltage, raw.voltage);
        

        // Errors / Fatals
        EXPECT_EQ(state.errors, raw.errors);
        EXPECT_EQ(state.fatals, raw.fatals);

        // Map
        EXPECT_EQ(state.currentMap, raw.current_map);

        // state raw
        EXPECT_EQ(state.state_raw, raw.state_raw);

        // battery
        raw.battery_level = 1.0;
        raw.charging = true;
        state = mapper_.toNavigationState(raw);
        EXPECT_EQ(state.battery.charge, domain::entities::NavigatorChargeState::Full);
        raw.battery_level = 0.5;
        raw.charging = true;
        state = mapper_.toNavigationState(raw);
        EXPECT_EQ(state.battery.charge, domain::entities::NavigatorChargeState::Charging);
        raw.battery_level = 0.6;
        raw.charging = false;
        state = mapper_.toNavigationState(raw);
        EXPECT_EQ(state.battery.charge, domain::entities::NavigatorChargeState::Unplugged);

        // emergency
        raw.emergency = false;
        raw.driver_emc = false;
        state = mapper_.toNavigationState(raw);
        EXPECT_FALSE(state.emergency);

        raw.emergency = true;
        raw.driver_emc = false;
        state = mapper_.toNavigationState(raw);
        EXPECT_TRUE(state.emergency);

        raw.emergency = false;
        raw.driver_emc = true;
        state = mapper_.toNavigationState(raw);
        EXPECT_TRUE(state.emergency);

        raw.emergency = true;
        raw.driver_emc = true;
        state = mapper_.toNavigationState(raw);
        EXPECT_TRUE(state.emergency);

        // task status
        for(int i = 0; i < 10; i++)
        {
            raw.task_status = i;
            state = mapper_.toNavigationState(raw);
            if(i > 0 && i < 7)
            {
                if(i == 1 || i == 2)
                {
                    EXPECT_EQ(state.taskStatus, domain::entities::NavigatorTaskState::Running);
                }
                else {
                    EXPECT_EQ(state.taskStatus, static_cast<domain::entities::NavigatorTaskState>(i));
                }
                
            }
            else {
                EXPECT_EQ(state.taskStatus, domain::entities::NavigatorTaskState::None);
            }
        }

        // location status
        for(int i = 0; i < 10; i ++)
        {
            raw.reloc_status = i;
            state = mapper_.toNavigationState(raw);
            if(i > 0 && i < 4)
            {
                EXPECT_EQ(state.locStatus, static_cast<domain::entities::NavigatorRelocationState>(i));
            }
            else {
                EXPECT_EQ(state.locStatus, domain::entities::NavigatorRelocationState::Unknown);
            }
        }
    }

    TEST_F(SeerNavigatorStateMapper_test, DetectEvents_CheckEmergency)
    {
        {
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";
            prev.emergencyStop = true;
            next.emergency = true;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());
            prev.emergencyStop = false;
            next.emergency = false;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());
        }
        {
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";
            prev.emergencyStop = false;
            next.emergency = true;
            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 1);
            EXPECT_TRUE(std::holds_alternative<domain::events::NavigatorSetEmergencyEvent>(events[0]));
            
        }
        {
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";
            prev.emergencyStop = true;
            next.emergency = false;
            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 1);
            EXPECT_TRUE(std::holds_alternative<domain::events::NavigatorClearEmergencyEvent>(events[0]));
        }
    }
    TEST_F(SeerNavigatorStateMapper_test, DetectEvents_CheckBlocked)
    {
        {
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev.blocked = true;
            next.blocked.detected = true;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());

            prev.blocked = false;
            next.blocked.detected = false;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());
        }

        {
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev.blocked = false;
            next.blocked.detected = true;
            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 1);
            EXPECT_TRUE(std::holds_alternative<domain::events::NavigatorSetBlockEvent>(events[0]));
        }

        {
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev.blocked = true;
            next.blocked.detected = false;
            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 1);
            EXPECT_TRUE(std::holds_alternative<domain::events::NavigatorClearBlockEvent>(events[0]));
        }
    }
    TEST_F(SeerNavigatorStateMapper_test, DetectEvents_CheckErrors)
    {
        {
            std::unordered_map<std::string, std::string> prev_errors;
            std::unordered_map<std::string, std::string> next_errors;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev.errors = prev_errors;
            next.errors = next_errors;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());

            prev_errors["5511"] = "desc1";
            prev_errors["5512"] = "desc2";
            prev_errors["5513"] = "desc3";
            next_errors["5511"] = "desc1";
            next_errors["5512"] = "desc2";
            next_errors["5513"] = "desc3";
            prev.errors = prev_errors;
            next.errors = next_errors;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());
        }

        {
            std::unordered_map<std::string, std::string> prev_errors;
            std::unordered_map<std::string, std::string> next_errors;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            next_errors["5513"] = "desc3";
            next_errors["5514"] = "desc4";
            next_errors["5515"] = "desc5";
            prev.errors = prev_errors;
            next.errors = next_errors;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 3);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorSetErrorEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(next.errors.count(pt->code), 1);
            }
        }

        {
            std::unordered_map<std::string, std::string> prev_errors;
            std::unordered_map<std::string, std::string> next_errors;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_errors["5515"] = "desc5";

            next_errors["5513"] = "desc3";
            next_errors["5514"] = "desc4";
            next_errors["5515"] = "desc5";
            prev.errors = prev_errors;
            next.errors = next_errors;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 2);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorSetErrorEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(next.errors.count(pt->code), 1);
                EXPECT_EQ(prev.errors.count(pt->code), 0);
            }
        }

        {
            std::unordered_map<std::string, std::string> prev_errors;
            std::unordered_map<std::string, std::string> next_errors;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_errors["5513"] = "desc3";
            prev_errors["5514"] = "desc4";
            prev_errors["5515"] = "desc5";
            prev.errors = prev_errors;
            next.errors = next_errors;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 3);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorClearErrorEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(prev.errors.count(pt->code), 1);
            }
        }

        {
            std::unordered_map<std::string, std::string> prev_errors;
            std::unordered_map<std::string, std::string> next_errors;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_errors["5513"] = "desc3";
            prev_errors["5514"] = "desc4";
            prev_errors["5515"] = "desc5";
            next_errors["5515"] = "desc5";
            prev.errors = prev_errors;
            next.errors = next_errors;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 2);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorClearErrorEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(prev.errors.count(pt->code), 1);
                EXPECT_EQ(next.errors.count(pt->code), 0);
            }
        }

        {
            std::unordered_map<std::string, std::string> prev_errors;
            std::unordered_map<std::string, std::string> next_errors;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_errors["5511"] = "desc1";
            prev_errors["5512"] = "desc2";
            prev_errors["5513"] = "desc3";
            next_errors["5513"] = "desc3";
            next_errors["5514"] = "desc4";
            next_errors["5515"] = "desc5";
            prev.errors = prev_errors;
            next.errors = next_errors;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 4);
            for(auto const& event:events)
            {
                std::visit([prev,next](const auto& e) 
                {
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, domain::events::NavigatorSetErrorEvent>)
                    {
                        EXPECT_EQ(prev.errors.count(e.code), 0);
                        EXPECT_EQ(next.errors.count(e.code), 1);
                    }
                    else if constexpr (std::is_same_v<T, domain::events::NavigatorClearErrorEvent>) {
                        EXPECT_EQ(prev.errors.count(e.code), 1);
                        EXPECT_EQ(next.errors.count(e.code), 0);
                    }
                    else {
                        FAIL();
                    }
                }, event);
            }
        }
    }
    TEST_F(SeerNavigatorStateMapper_test, DetectEvents_CheckFatals)
    {
        {
            std::unordered_map<std::string, std::string> prev_fatals;
            std::unordered_map<std::string, std::string> next_fatals;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev.fatals = prev_fatals;
            next.fatals = next_fatals;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());

            prev_fatals["5511"] = "desc1";
            prev_fatals["5512"] = "desc2";
            prev_fatals["5513"] = "desc3";
            next_fatals["5511"] = "desc1";
            next_fatals["5512"] = "desc2";
            next_fatals["5513"] = "desc3";
            prev.fatals = prev_fatals;
            next.fatals = next_fatals;
            EXPECT_TRUE(mapper_.detectEvents(prev, next, taskId).empty());
        }

        {
            std::unordered_map<std::string, std::string> prev_fatals;
            std::unordered_map<std::string, std::string> next_fatals;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            next_fatals["5513"] = "desc3";
            next_fatals["5514"] = "desc4";
            next_fatals["5515"] = "desc5";
            prev.fatals = prev_fatals;
            next.fatals = next_fatals;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 3);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorSetFatalEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(next.fatals.count(pt->code), 1);
            }
        }

        {
            std::unordered_map<std::string, std::string> prev_fatals;
            std::unordered_map<std::string, std::string> next_fatals;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_fatals["5515"] = "desc5";

            next_fatals["5513"] = "desc3";
            next_fatals["5514"] = "desc4";
            next_fatals["5515"] = "desc5";
            prev.fatals = prev_fatals;
            next.fatals = next_fatals;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 2);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorSetFatalEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(next.fatals.count(pt->code), 1);
                EXPECT_EQ(prev.fatals.count(pt->code), 0);
            }
        }
    
        {
            std::unordered_map<std::string, std::string> prev_fatals;
            std::unordered_map<std::string, std::string> next_fatals;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_fatals["5513"] = "desc3";
            prev_fatals["5514"] = "desc4";
            prev_fatals["5515"] = "desc5";
            prev.fatals = prev_fatals;
            next.fatals = next_fatals;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 3);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorClearFatalEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(prev.fatals.count(pt->code), 1);
            }
        }

        {
            std::unordered_map<std::string, std::string> prev_fatals;
            std::unordered_map<std::string, std::string> next_fatals;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_fatals["5513"] = "desc3";
            prev_fatals["5514"] = "desc4";
            prev_fatals["5515"] = "desc5";
            next_fatals["5515"] = "desc5";
            prev.fatals = prev_fatals;
            next.fatals = next_fatals;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 2);
            for(auto const& event:events)
            {
                auto pt = std::get_if<domain::events::NavigatorClearFatalEvent>(&event);
                ASSERT_NE(pt, nullptr);
                EXPECT_EQ(prev.fatals.count(pt->code), 1);
                EXPECT_EQ(next.fatals.count(pt->code), 0);
            }
        }

        {
            std::unordered_map<std::string, std::string> prev_fatals;
            std::unordered_map<std::string, std::string> next_fatals;
            auto prev = makeDefaultPrevSnapshot();
            auto next = makeDefaultNavigatorState();
            std::string taskId = "123";

            prev_fatals["5511"] = "desc1";
            prev_fatals["5512"] = "desc2";
            prev_fatals["5513"] = "desc3";
            next_fatals["5513"] = "desc3";
            next_fatals["5514"] = "desc4";
            next_fatals["5515"] = "desc5";
            prev.fatals = prev_fatals;
            next.fatals = next_fatals;

            auto events = mapper_.detectEvents(prev, next, taskId);
            ASSERT_EQ(events.size(), 4);
            for(auto const& event:events)
            {
                std::visit([prev,next](const auto& e) 
                {
                    using T = std::decay_t<decltype(e)>;
                    if constexpr (std::is_same_v<T, domain::events::NavigatorSetFatalEvent>)
                    {
                        EXPECT_EQ(prev.fatals.count(e.code), 0);
                        EXPECT_EQ(next.fatals.count(e.code), 1);
                    }
                    else if constexpr (std::is_same_v<T, domain::events::NavigatorClearFatalEvent>) {
                        EXPECT_EQ(prev.fatals.count(e.code), 1);
                        EXPECT_EQ(next.fatals.count(e.code), 0);
                    }
                    else {
                        FAIL();
                    }
                }, event);
            }
        }
    }

}