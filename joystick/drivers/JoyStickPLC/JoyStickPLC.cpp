#include "../joystick/drivers/JoyStickPLC/JoyStickPLC.hpp"
#include <iostream>
#include <jsoncpp/json/json.h>
#include <mutex>

namespace joystick::drivers::plc {
    JoyStickPLC::JoyStickPLC()
    {

    }
    void JoyStickPLC::setJoyEventCallback(JoyStickEventCallback cb)
    {
        eventCallback_ = std::move(cb);
    }
    void JoyStickPLC::updateState(const std::string& raw_state)
    {
        Json::Value root;
        Json::CharReaderBuilder builder;
        Json::CharReader* reader = builder.newCharReader();
        std::string errors;
        
        // Parse JSON
        bool success = reader->parse(
            raw_state.c_str(),
            raw_state.c_str() + raw_state.size(),
            &root,
            &errors
        );

        delete reader;

        if (!success) {
            std::cerr << "Failed to parse JSON: " << errors << std::endl;
        }
        else {
            joystick::domain::entities::JoyStickSignal prev;
            joystick::domain::entities::JoyStickSignal next;

            {
                std::lock_guard<std::mutex> lk(mutexSignal_);
                prev = prevSnapshot_;
                int input_id = 0;
                if(root.isMember("inputs") && root["inputs"].isArray())
                {
                    for(const auto& item : root["inputs"])
                    {   
                        switch (input_id) {
                            case 0:
                            {
                                if(item.isUInt())
                                {
                                    if(item.asUInt() == 1)
                                    {
                                        cacheSignal_.turn_left = false;
                                    }
                                    else {
                                        cacheSignal_.turn_left = true;
                                    }
                                }
                                break;
                            }
                            case 1:
                            {
                                if(item.isUInt())
                                {
                                    if(item.asUInt() == 1)
                                    {
                                        cacheSignal_.turn_right = false;
                                    }
                                    else {
                                        cacheSignal_.turn_right = true;
                                    }
                                }
                                break;
                            }
                            case 2:
                            {
                                if(item.isUInt())
                                {
                                    if(item.asUInt() == 1)
                                    {
                                        cacheSignal_.forward = false;
                                    }
                                    else {
                                        cacheSignal_.forward = true;
                                    }
                                }
                                break;
                            }
                            case 3:
                            {
                                if(item.isUInt())
                                {
                                    if(item.asUInt() == 1)
                                    {
                                        cacheSignal_.backward = false;
                                    }
                                    else {
                                        cacheSignal_.backward = true;
                                    }
                                }
                                break;
                            }
                            case 4:
                            {
                                if(item.isUInt())
                                {
                                    if(item.asUInt() == 1)
                                    {
                                        cacheSignal_.enable = false;
                                    }
                                    else {
                                        cacheSignal_.enable = true;
                                    }
                                }
                                break;
                            }

                            default:
                            {
                                break;
                            }
                        }
                        input_id++;
                    }
                }
                next = cacheSignal_;
                prevSnapshot_ = cacheSignal_;
                detectEvent(prev, next);
            }
        }
    }

    void JoyStickPLC::detectEvent(const joystick::domain::entities::JoyStickSignal& prev, const joystick::domain::entities::JoyStickSignal& next)
    {
        if(eventCallback_)
        {
            if(!prev.turn_left && next.turn_left)
            {
                eventCallback_(joystick::domain::events::JoyStickSetTurnLeftEvent{});
            }
            else if(prev.turn_left && !next.turn_left)
            {
                eventCallback_(joystick::domain::events::JoyStickClearTurnLeftEvent{});
            }

            if(!prev.turn_right && next.turn_right)
            {
                eventCallback_(joystick::domain::events::JoyStickSetTurnRightEvent{});
            }
            else if(prev.turn_right && !next.turn_right)
            {
                eventCallback_(joystick::domain::events::JoyStickClearTurnRightEvent{});
            }

            if(!prev.forward && next.forward)
            {
                eventCallback_(joystick::domain::events::JoyStickSetForwardEvent{});
            }
            else if(prev.forward && !next.forward)
            {
                eventCallback_(joystick::domain::events::JoyStickClearForwardEvent{});
            }

            if(!prev.backward && next.backward)
            {
                eventCallback_(joystick::domain::events::JoyStickSetBackwardEvent{});
            }
            else if(prev.backward && !next.backward)
            {
                eventCallback_(joystick::domain::events::JoyStickClearBackwardEvent{});
            }

            if(!prev.enable && next.enable)
            {
                eventCallback_(joystick::domain::events::JoyStickEnableEvent{});
            }
            else if(prev.enable && !next.enable)
            {
                eventCallback_(joystick::domain::events::JoyStickDisableEvent{});
            }
        }
    }
}