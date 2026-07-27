#pragma once
#include "SeerNavigatorFrameCodec.hpp"
#include "NavigatorState.hpp"
#include <string>
#include <optional>
#include <cstdint>

namespace navigator::drivers::seer {
    enum class SeerNavigatorMessageNumber : uint16_t {
        // Request
        StatusAll1Req = 0x044C, // port 19204

        GoTargetReq   = 0x0BEB, // port 19206
        PauseNavReq    = 0x0BB9, // port 19206
        ResumeNavReq   = 0x0BBA, // port 19206
        CancelNavReq   = 0x0BBB, // port 19206

        RelocationReq = 0x07D2, // port 19205
        ConfirmCorrectRelocationReq = 0x07D3, // port 19205
        OpenLoopMotionReq = 0x07DA, // port 19205

        PlayAudioReq = 0x1770,
        StopAudioReq = 0x177C,
        PauseAudioReq = 0x177A,
        ResumeAudioReq = 0x177B,

        // Response (Request + 0x2710)
        StatusAll1Res = 0x2B5C,

        GoTargetRes   = 0x32FB,  // 13051
        PauseNavRes   = 0x32C9,  // 13001
        ResumeNavRes  = 0x32CA,  // 13002
        CancelNavRes  = 0x32CB,  // 13003

        RelocationRes = 0x2EE2,
        ConfirmCorrectRelocationRes = 0x22E3,
        OpenLoopMotionRes = 0x2EEA,

        PlayAudioRes = 0x3E80,
        StopPlayingAudioRes = 0x3E8C,
        PauseAudioRes = 0x3E8A,
        ResumeAudioRes = 0x3E8B
    };

    struct GoTargetOptions {
        std::string sourceId = "SELF_POSITION";
        std::string taskId;
        std::optional<double>  angle;                         // rad, world coordinate
        std::optional<std::string> method;                    // "forward" | "backward"
        std::optional<double>  maxSpeed;                      // m/s
        std::optional<double>  maxWspeed;                     // rad/s
        std::optional<double>  maxAcc;                        // m/s²
        std::optional<double>  maxWacc;                       // rad/s²
        std::optional<double>  duration;                      // ms, wait after arrival
    };

    class SeerNavigatorCommandBuilder {
        public:
            SeerNavigatorCommandBuilder() = default;

            navigator::drivers::seer::SeerNavigatorFrame goToStation(const std::string& targetId,const GoTargetOptions& opts = {});
            navigator::drivers::seer::SeerNavigatorFrame goToPoint(double x,double y, double theta, navigator::domain::entities::NavigatorBackMode back_mode,navigator::domain::entities::NavigatorCoordinate coordinate, const std::string& taskId = "");

            navigator::drivers::seer::SeerNavigatorFrame pauseNavigation();
            navigator::drivers::seer::SeerNavigatorFrame resumeNavigation();
            navigator::drivers::seer::SeerNavigatorFrame cancelNavigation();

            navigator::drivers::seer::SeerNavigatorFrame statusAll1(bool returnLaser = false, bool returnBeams3D = false, int timeoutMs = 500);

            navigator::drivers::seer::SeerNavigatorFrame relocation(double x, double y, double angle);
            navigator::drivers::seer::SeerNavigatorFrame confirmLocation();
            navigator::drivers::seer::SeerNavigatorFrame openLoopMotion(double vx,double vy, double w, uint32_t duration_ms);

            navigator::drivers::seer::SeerNavigatorFrame playAudio(const std::string& nameAudio);
            navigator::drivers::seer::SeerNavigatorFrame stopAudio();
            navigator::drivers::seer::SeerNavigatorFrame pauseAudio();
            navigator::drivers::seer::SeerNavigatorFrame resumeAudio();

        private:
            uint16_t nextSerial();
            uint16_t serial_ = 0;

            std::string buildGoTargetJson(const std::string& targetId,
                                          const std::string& sourceId,
                                          const std::string& taskId,
                                          const GoTargetOptions& opts);
            std::string generateTaskId(const std::string& prefix);
    };
}