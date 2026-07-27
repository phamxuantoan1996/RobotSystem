#include "SeerNavigatorCommandBuilder.hpp"
#include "SeerNavigatorFrameCodec.hpp"
#include "NavigatorState.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace navigator::drivers::seer {
    uint16_t SeerNavigatorCommandBuilder::nextSerial()
    {
        serial_++;
        if(serial_ == 65535)
            serial_ = 0;
        return serial_;
    }

    std::string SeerNavigatorCommandBuilder::generateTaskId(const std::string& prefix)
    {
        auto ns = std::chrono::steady_clock::now().time_since_epoch().count();
        return prefix + std::to_string(ns);
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::goToStation(const std::string& targetId,const GoTargetOptions& opts)
    {
        const std::string taskId = opts.taskId.empty()
            ? generateTaskId("nav_")
            : opts.taskId;

        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetReq);
        frame.payload = buildGoTargetJson(targetId, opts.sourceId, taskId, opts);
        return frame;
    }

    SeerNavigatorFrame SeerNavigatorCommandBuilder::goToPoint(double x,double y, double theta,navigator::domain::entities::NavigatorBackMode back_mode,navigator::domain::entities::NavigatorCoordinate coordinate, const std::string& taskId)
    {
        const std::string tid = taskId.empty() ? generateTaskId("pt_") : taskId;
        int back = back_mode == navigator::domain::entities::NavigatorBackMode::Backward ? 1 : 0;
        std::string coor = coordinate == navigator::domain::entities::NavigatorCoordinate::WORLD ? "world" : "robot";
        std::ostringstream json;
        json << std::fixed << std::setprecision(8);
        json << "{"
         << "\"script_name\":\"syspy/goPath.py\","
         << "\"script_args\":{"
            << "\"x\":" << x << ","
            << "\"y\":" << y << ","
            << "\"theta\":" << theta << ","
            << "\"backMode\":" << back << ","
            << "\"coordinate\":\"" << coor << "\""
         << "},"
         << "\"operation\":\"Script\","
         << "\"id\":\"SELF_POSITION\","
         << "\"source_id\":\"SELF_POSITION\","
         << "\"task_id\":\"" << tid << "\""
         << "}";
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::GoTargetReq);
        frame.payload = json.str();
        return frame;
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::pauseNavigation()
    {
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseNavReq);
        return frame;
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::resumeNavigation()
    {
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::ResumeNavReq);
        return frame;
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::cancelNavigation()
    {
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::CancelNavReq);
        return frame;
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::relocation(double x, double y, double angle)
    {
        double length = 1;
        std::ostringstream json;
        json << std::fixed << std::setprecision(8);
        json << "{"
            << "\"x\":"      << x      << ","
            << "\"y\":"      << y      << ","
            << "\"angle\":"  << angle  << ","
            << "\"length\":" << length
            << "}";

        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::RelocationReq);
        frame.payload = json.str();
        return frame;
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::statusAll1(
        bool returnLaser,
        bool returnBeams3D,
        int  timeoutMs)
    {
        std::ostringstream json;
        json << "{"
            << "\"return_laser\":"    << (returnLaser   ? "true" : "false") << ","
            << "\"return_beams3D\":"  << (returnBeams3D ? "true" : "false") << ","
            << "\"timeout\":"         << timeoutMs
            << "}";

        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::StatusAll1Req);
        frame.payload = json.str();
        return frame;
    }

    std::string SeerNavigatorCommandBuilder::buildGoTargetJson(
        const std::string&     targetId,
        const std::string&     sourceId,
        const std::string&     taskId,
        const GoTargetOptions& opts)
    {
        std::ostringstream json;
        json << std::fixed << std::setprecision(4);

        json << "{"
            << "\"id\":\""         << targetId << "\","
            << "\"source_id\":\"" << sourceId  << "\","
            << "\"task_id\":\""   << taskId    << "\"";

        if (opts.angle.has_value())
            json << ",\"angle\":"      << *opts.angle;

        if (opts.method.has_value())
            json << ",\"method\":\""   << *opts.method << "\"";

        if (opts.maxSpeed.has_value())
            json << ",\"max_speed\":"  << *opts.maxSpeed;

        if (opts.maxWspeed.has_value())
            json << ",\"max_wspeed\":" << *opts.maxWspeed;

        if (opts.maxAcc.has_value())
            json << ",\"max_acc\":"    << *opts.maxAcc;

        if (opts.maxWacc.has_value())
            json << ",\"max_wacc\":"   << *opts.maxWacc;

        if (opts.duration.has_value())
            json << ",\"duration\":"   << *opts.duration;

        json << "}";
        return json.str();
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::confirmLocation(void)
    {
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::ConfirmCorrectRelocationReq);
        return frame;
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::openLoopMotion(double vx,double vy, double w,uint32_t duration_ms)
    {
        std::ostringstream json;
        json << std::fixed << std::setprecision(4);
        json << "{"
            << "\"vx\":"       << vx          << ","
            << "\"vy\":"       << vy          << ","
            << "\"w\":"        << w           << ","
            << "\"duration\":" << duration_ms
            << "}";

        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::OpenLoopMotionReq);
        frame.payload = json.str();
        return frame;
    }

    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::playAudio(const std::string& nameAudio)
    {
        // {"name":"collision","loop":true}
        std::ostringstream json;
        json << std::fixed << std::setprecision(4);
        json << "{"
            << "\"name\":\"" << nameAudio << "\","
            << "\"loop\":true"
            << "}";

        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::PlayAudioReq);
        frame.payload = json.str();
        return frame;
    }
    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::stopAudio()
    {
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::StopAudioReq);
        return frame;
    }
    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::pauseAudio()
    {
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::PauseAudioReq);
        return frame;
    }
    navigator::drivers::seer::SeerNavigatorFrame SeerNavigatorCommandBuilder::resumeAudio()
    {
        navigator::drivers::seer::SeerNavigatorFrame frame;
        frame.serial  = nextSerial();
        frame.msgType = static_cast<uint16_t>(SeerNavigatorMessageNumber::ResumeAudioReq);
        return frame;
    }
}