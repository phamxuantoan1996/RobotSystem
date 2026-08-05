#include "SeerNavigatorDriverReal.hpp"
#include "NavigatorEvent.hpp"
#include "NavigatorState.hpp"
#include "SeerNavigatorCommandBuilder.hpp"
#include "SeerNavigatorConnectionReal.hpp"
#include <iostream>
#include <jsoncpp/json/json.h>
#include <memory>
#include <mutex>

namespace navigator::drivers::seer {
    SeerNavigatorDriverReal::SeerNavigatorDriverReal(SeerNavigatorDriverConfigParams configParams) : configParams_(std::move(configParams))
    {
        if(configParams_.mode == "test")
        {

        }
        else if(configParams.mode == "sim")
        {

        }
        else { // real
            statusConn_ = std::make_unique<navigator::drivers::seer::SeerNavigatorConnectionReal>(configParams_.host,configParams_.statusPort,configParams_.timeout);
            navConn_ = std::make_unique<navigator::drivers::seer::SeerNavigatorConnectionReal>(configParams_.host,configParams_.navPort,configParams_.timeout);
            controlConn_ = std::make_unique<navigator::drivers::seer::SeerNavigatorConnectionReal>(configParams_.host,configParams_.controlPort,configParams_.timeout);
            otherConn_ = std::make_unique<navigator::drivers::seer::SeerNavigatorConnectionReal>(configParams_.host,configParams_.otherPort,configParams_.timeout);
        }
    }
    SeerNavigatorDriverReal::~SeerNavigatorDriverReal()
    {
        disconnect();
    }

    std::error_code SeerNavigatorDriverReal::connect()
    {
        bool expected = false;
        if(!running_.compare_exchange_strong(expected,true))
        {
            running_ = false;
            interruptPoll_ = true;
            statusConn_->disconnect();
            // Disconnect the all old connections
            statusConn_->disconnect();
            navConn_->disconnect();
            controlConn_->disconnect();
            otherConn_->disconnect();
            if(workerThread_.joinable())
            {
                workerThread_.join();
            }
        }
        connected_ = false;
        
        if(auto ec = statusConn_->connect())
        {
            return ec;
        }

        if(auto ec = navConn_->connect())
        {
            statusConn_->disconnect();
            return ec;
        }

        if(auto ec = controlConn_->connect())
        {
            statusConn_->disconnect();
            navConn_->disconnect();
            return ec;
        }

        if(auto ec = otherConn_->connect())
        {
            statusConn_->disconnect();
            navConn_->disconnect();
            controlConn_->disconnect();
            return ec;
        }

        connected_ = true;
        running_ = true;
        workerThread_ = std::thread(&SeerNavigatorDriverReal::workerTask,this);
        return {};
    }
    void SeerNavigatorDriverReal::disconnect()
    {
        running_ = false;
        if(workerThread_.joinable())
        {
            workerThread_.join();
        }
        statusConn_->disconnect();
        navConn_->disconnect();
        controlConn_->disconnect();
        otherConn_->disconnect();
        connected_ = false;
    }
    bool SeerNavigatorDriverReal::isConnected() const
    {
        return connected_;
    }

    // send Navigation Command + await ACK
    std::error_code SeerNavigatorDriverReal::sendSeerNavigationCommand(const navigator::drivers::seer::SeerNavigatorFrame & req, uint16_t expectedResType)
    {
        std::lock_guard<std::mutex> lk(navMutex_);
        auto res = navConn_->sendRequest(req);

        if (!res.has_value())
            return std::make_error_code(std::errc::timed_out);

        if (res->msgType != expectedResType)
            return std::make_error_code(std::errc::protocol_error);

        // ret_code == 0 or missing means success (per SEER doc)
        if (parseRetCode(res->payload) != 0)
            return std::make_error_code(std::errc::protocol_error);
        return {};
    }
    // send Control Command + await ACK
    std::error_code SeerNavigatorDriverReal::sendControlCommand(const navigator::drivers::seer::SeerNavigatorFrame& req, uint16_t expectedResType)
    {
        std::lock_guard<std::mutex> lk(controlMutex_);
        auto res = controlConn_->sendRequest(req);

        if (!res.has_value())
            return std::make_error_code(std::errc::timed_out);

        if (res->msgType != expectedResType)
            return std::make_error_code(std::errc::protocol_error);

        // ret_code == 0 or missing means success (per SEER doc)
        if (parseRetCode(res->payload) != 0)
            return std::make_error_code(std::errc::protocol_error);
        return {};
    }
    // send Other Command + await ACK
    std::error_code SeerNavigatorDriverReal::SeerNavigatorDriverReal::sendOtherCommand(const navigator::drivers::seer::SeerNavigatorFrame& req, uint16_t expectedResType)
    {
        std::lock_guard<std::mutex> lk(otherMutex_);
        auto res = otherConn_->sendRequest(req);

        if (!res.has_value())
            return std::make_error_code(std::errc::timed_out);

        if (res->msgType != expectedResType)
            return std::make_error_code(std::errc::protocol_error);

        // ret_code == 0 or missing means success (per SEER doc)
        if (parseRetCode(res->payload) != 0)
            return std::make_error_code(std::errc::protocol_error);
        return {};
    }
    // ── JSON helpers ──────────────────────────────────────────────────────────
    int SeerNavigatorDriverReal::parseRetCode(const std::string& json)
    {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        bool parsingSuccessful = reader->parse(
            json.c_str(), 
            json.c_str() + json.length(), 
            &root, 
            &errors
        );
        if (parsingSuccessful)
        {
            if(root.isMember("ret_code") && root["ret_code"].isInt())
            {
                return root["ret_code"].asInt();
            }
        }
        return 1;
    }
    std::string SeerNavigatorDriverReal::extractTaskId(const std::string& json)
    {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        bool parsingSuccessful = reader->parse(
            json.c_str(), 
            json.c_str() + json.length(), 
            &root, 
            &errors
        );
        if (parsingSuccessful)
        {
            if(root.isMember("task_id") && root["task_id"].isString())
            {
                return root["task_id"].asString();
            }
        }
        return "";
    }
    SeerStatusAll1 SeerNavigatorDriverReal::parseAll1Json(const std::string& json)
    {
        SeerStatusAll1 s;
        Json::Value root;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::istringstream json_output(json);
        if (!Json::parseFromStream(reader, json_output, &root, &errs)) 
        {
            std::cout << "Parse error: " << errs << std::endl;
        }
        else
        {
            // pose
            if(root["x"].isDouble())
                s.x = root["x"].asDouble();
            else
                s.x = 0.0;
            if(root["y"].isDouble())
                s.y = root["y"].asDouble();
            else
                s.y = 0.0;
            if(root["angle"].isDouble())
                s.angle = root["angle"].asDouble();
            else
                s.angle = 0.0;

            // station
            if(root["current_station"].isString())
                s.current_station = root["current_station"].asString();
            else
                s.current_station = "";
            if(root["last_station"].isString())
                s.last_station = root["last_station"].asString();
            else
                s.last_station = "";

            // Velocity
            if(root["vx"].isDouble())
                s.vx = root["vx"].asDouble();
            else
                s.vx = 0.0;
            if(root["vy"].isDouble())
                s.vy = root["vy"].asDouble();
            else
                s.vy = 0.0;
            if(root["w"].isDouble())
                s.w = root["w"].asDouble();
            else
                s.w = 0.0;
            if(root["is_stop"].isBool())
                s.is_stop = root["is_stop"].asBool();
            else
                s.is_stop = false;

            // Obstacle — blocked
            if(root["blocked"].isBool())
                s.blocked = root["blocked"].asBool();
            else
                s.blocked = false;
            if(root["block_reason"].isInt())
                s.block_reason = root["block_reason"].asInt();
            else
                s.block_reason = 0;
            if(root["block_x"].isDouble())
                s.block_x = root["block_x"].asDouble();
            else
                s.block_x = 0.0;
            if(root["block_y"].isDouble())
                s.block_y = root["block_y"].asDouble();
            else
                s.block_y = 0.0;

            // Obstacle — slowed
            if(root["slowed"].isBool())
                s.slowed = root["slowed"].asBool();
            else
                s.slowed = false;
            if(root["slow_reason"].isInt())
                s.slow_reason = root["slow_reason"].asInt();
            else
                s.slow_reason = 0;
            if(root["slow_x"].isDouble())
                s.slow_x = root["slow_x"].asDouble();
            else
                s.slow_x = 0.0;
            if(root["slow_y"].isDouble())
                s.slow_y = root["slow_y"].asDouble();
            else
                s.slow_y = 0.0;

            // Battery
            if(root["battery_level"].isDouble())
                s.battery_level = root["battery_level"].asDouble();
            else
                s.battery_level = 0.0;
            if(root["voltage"].isDouble())
                s.voltage = root["voltage"].asDouble();
            else
                s.voltage = 0.0;
            if(root["current"].isDouble())
                s.current = root["current"].asDouble();
            else
                s.current = 0.0;
            if(root["charging"].isBool())
                s.charging = root["charging"].asBool();
            else
                s.charging = false;

            // Task / navigation
            if(root["task_status"].isInt())
                s.task_status = root["task_status"].asInt();
            else
                s.task_status = 0;
            if(root["target_id"].isString())
                s.target_id = root["target_id"].asString();
            else
                s.target_id = "";

            // System
            if(root["reloc_status"].isInt())
                s.reloc_status = root["reloc_status"].asInt();
            else
                s.reloc_status = 0;
            if(root["current_map"].isString())
                s.current_map = root["current_map"].asString();
            else
                s.current_map = "";
            if(root["emergency"].isBool())
                s.emergency = root["emergency"].asBool();
            else
                s.emergency = false;
            if(root["driver_emc"].isBool())
                s.driver_emc = root["driver_emc"].asBool();
            else
                s.driver_emc = false;

            // unfinished_path and finished_path — array of station ID strings
            // Format: "unfinished_path":["LM17","LM18","LM19"]
            // Only valid when task_type == 3 or 2
            if (root.isMember("unfinished_path") && root["unfinished_path"].isArray()) {
                for (const auto& item : root["unfinished_path"]) {
                    if (item.isString()) {
                        s.unfinished_path.push_back(item.asString());
                    }
                }
            }
            if (root.isMember("finished_path") && root["finished_path"].isArray()) {
                for (const auto& item : root["finished_path"]) {
                    if (item.isString()) {
                        s.finished_path.push_back(item.asString());
                    }
                }
            }
            // raw
            s.state_raw = json;
            
            // errors
            if(root.isMember("errors") && root["errors"].isArray())
            {
                const Json::Value errors = root["errors"];
                for (Json::Value::ArrayIndex i = 0; i < errors.size(); ++i) 
                {
                    const Json::Value error = errors[i];
                    // lay error code
                    std::string errorCode = "";
                    Json::Value::Members keys = error.getMemberNames();
                    for (const std::string& key : keys) 
                    {
                        // Nếu key KHÔNG PHẢI là "desc" và cũng KHÔNG PHẢI là "times"
                        if (key != "desc" && key != "times") 
                        {
                            // Đây chính là mã lỗi bạn cần (ví dụ: "52201" hoặc "52118")
                            errorCode = key; 
                        }
                    }
                    // lay desc error
                    std::string descError = "";
                    if(error.isMember("desc") && error["desc"].isString())
                    {
                        descError = error["desc"].asString();
                    }
                    if(!errorCode.empty() && !descError.empty())
                    {
                        s.errors[errorCode] = descError;
                    }
                }
            }
            // fatals
            if(root.isMember("fatals") && root["fatals"].isArray())
            {
                const Json::Value fatals = root["fatals"];
                for (Json::Value::ArrayIndex i = 0; i < fatals.size(); ++i) 
                {
                    const Json::Value fatal = fatals[i];
                    // lay error code
                    std::string fatalCode = "";
                    Json::Value::Members keys = fatal.getMemberNames();
                    for (const std::string& key : keys) 
                    {
                        // Nếu key KHÔNG PHẢI là "desc" và cũng KHÔNG PHẢI là "times"
                        if (key != "desc" && key != "times") 
                        {
                            // Đây chính là mã lỗi bạn cần (ví dụ: "52201" hoặc "52118")
                            fatalCode = key; 
                        }
                    }
                    // lay desc error
                    std::string descError = "";
                    if(fatal.isMember("desc") && fatal["desc"].isString())
                    {
                        descError = fatal["desc"].asString();
                    }
                    if(!fatalCode.empty() && !descError.empty())
                    {
                        s.errors[fatalCode] = descError;
                    }
                }
            }
        }
        return s;
    }

    // workerTask
    void SeerNavigatorDriverReal::workerTask()
    {   
        
        while (running_) 
        {
            interruptPoll_ = false;
            const auto start = std::chrono::steady_clock::now();
            auto req = cmdBuilder_.statusAll1(false,false,configParams_.pollStatusIntervals);
            auto res = statusConn_->sendRequest(req);
            if(!running_ || interruptPoll_)
            {
                break;
            }
            
            if(!res)
            {
                if(connected_.exchange(false))
                {
                    if(eventCallback_)
                    {
                        eventCallback_(domain::events::NavigatorDisconnectEvent{});
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(configParams_.pollStatusIntervals));
                }
                continue;
            }
            
            if(!running_)
            {
                break;
            }
            if(!connected_.exchange(true))
            {
                if(eventCallback_)
                {
                    eventCallback_(domain::events::NavigatorReconnectEvent{});
                }
            }
            if(res.has_value() && !res->payload.empty())
            {
                // std::cout << res->payload << "\n";
                SeerStatusAll1 raw   = parseAll1Json(res->payload);
                auto           next  = stateMapper_.toNavigationState(raw);

                std::string  activeTaskId;
                PrevSnapshot prev;
                {
                    std::lock_guard<std::mutex> lk(mutexState_);
                    prev          = prevSnapshot_;
                    activeTaskId  = activeTaskId_;
                    state_  = next;
                    prevSnapshot_ = stateMapper_.snapshotOf(next);
                }

                auto events = stateMapper_.detectEvents(prev, next, activeTaskId);
                if (eventCallback_) {
                    for (auto& e : events)
                        eventCallback_(e);
                }
            }

            // Sleep for the remainder of the interval
            const auto elapsed   = std::chrono::steady_clock::now() - start;
            const auto remaining = std::chrono::milliseconds(configParams_.pollStatusIntervals) - elapsed;
            if (remaining > std::chrono::milliseconds(0))
                std::this_thread::sleep_for(remaining);
        
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // goToStation
    // ─────────────────────────────────────────────────────────────────────────────
    std::error_code SeerNavigatorDriverReal::goToStation(const navigator::domain::value_objects::Station& station)
    {
        auto frame = cmdBuilder_.goToStation(station.getId());

        auto taskId = extractTaskId(frame.payload);
        stateMapper_.setExpectTargetId(taskId,station.getId());
        
        auto erc = sendSeerNavigationCommand(frame, static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetRes));
        {
            std::lock_guard<std::mutex> lk(mutexState_);
            activeTaskId_ = taskId;
        }
        return erc;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // goToPoint
    // ─────────────────────────────────────────────────────────────────────────────
    std::error_code SeerNavigatorDriverReal::goToPoint(const navigator::domain::value_objects::Location& location,domain::entities::NavigatorBackMode backMode, domain::entities::NavigatorCoordinate navigatorCoordinate)
    {
        auto frame = cmdBuilder_.goToPoint(location.getX(), location.getY(), location.getAngle(),backMode,navigatorCoordinate,"task_id_"+std::to_string(location.getX())+"_"+std::to_string(location.getY()) + "_" + std::to_string(location.getY()));
        // Lưu target pose để checkArrived so sánh bằng tọa độ
        // Reset GoToStation target đồng thời
        domain::entities::NavigatorPose pose0;
        {
            std::lock_guard<std::mutex> lk(mutexState_);
            pose0 = state_.pose;
        }
        stateMapper_.setGoToPointTarget(location.getX(), location.getY(), location.getAngle(), pose0,navigatorCoordinate);
        {
            std::lock_guard<std::mutex> lk(mutexState_);
            activeTaskId_ = extractTaskId(frame.payload);
            // std::cout << "active id : " << activeTaskId_ << "\n";
        }
        return sendSeerNavigationCommand(frame, static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetRes));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // pauseNavigation / resumeNavigation / cancelNavigation
    // ─────────────────────────────────────────────────────────────────────────────
    std::error_code SeerNavigatorDriverReal::pauseNavigation()
    {
        return sendSeerNavigationCommand(cmdBuilder_.pauseNavigation(),
                        static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseNavRes));
    }

    std::error_code SeerNavigatorDriverReal::resumeNavigation()
    {
        return sendSeerNavigationCommand(cmdBuilder_.resumeNavigation(),
                        static_cast<uint16_t>(SeerNavigatorMessageNumber::ResumeNavRes));
    }

    std::error_code SeerNavigatorDriverReal::cancelNavigation()
    {
        auto ec = sendSeerNavigationCommand(cmdBuilder_.cancelNavigation(),
                            static_cast<uint16_t>(SeerNavigatorMessageNumber::CancelNavRes));
        if (!ec) 
        {
            std::lock_guard<std::mutex> lk(mutexState_);
            activeTaskId_.clear();
        }
        return ec;
    }

    std::error_code SeerNavigatorDriverReal::relocation(const navigator::domain::value_objects::Location& location)
    {
        auto frame = cmdBuilder_.relocation(location.getX(),location.getY(),location.getAngle());
        return sendControlCommand(frame, static_cast<uint16_t>(SeerNavigatorMessageNumber::RelocationRes));
    }
    std::error_code SeerNavigatorDriverReal::confirmRelocation()
    {
        auto frame = cmdBuilder_.confirmLocation();
        return sendControlCommand(frame, static_cast<uint16_t>(SeerNavigatorMessageNumber::ConfirmCorrectRelocationRes));
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // getState / setEventCallback
    // ─────────────────────────────────────────────────────────────────────────────
    domain::entities::NavigatorState SeerNavigatorDriverReal::getState() const
    {
        std::lock_guard<std::mutex> lk(mutexState_);
        return state_;
    }
    void SeerNavigatorDriverReal::setNavigatorEventCallback(NavigatorEventCallback cb)
    {
        eventCallback_ = std::move(cb);
    }
}