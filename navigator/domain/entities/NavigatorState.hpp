#pragma once
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
namespace navigator::domain::entities {
    enum class NavigatorCoordinate {
        SELF,
        WORLD
    };

    enum class NavigatorBackMode {
        Forward,
        Backward
    };

    enum class NavigatorRelocationState {
        Unknown, 
        Localised, 
        Relocalising, 
        LocalisedConfirm
    };

    enum class NavigatorChargeState { 
        Unplugged, 
        Charging, 
        Full 
    };

    enum class NavigatorTaskState {
        None,
        Waiting,
        Running,
        Suspended,
        Completed,
        Failed,
        Canceled
    };

    struct NavigatorPose {
        double x          = 0;
        double y          = 0;
        double angle      = 0;    // rad
        double confidence = 0;    // [0, 1]
    };

    struct NavigatorObstacleInfo {
        bool   detected = false;
        int    reason   = -1;     // vendor-specific code, -1 = none
        double x        = 0;
        double y        = 0;
    };

    struct NavigatorBatteryInfo {
        double      level   = 0;  // [0, 1]
        double      voltage = 0;  // V
        NavigatorChargeState charge  = NavigatorChargeState::Unplugged;
    };

    struct NavigatorState {
        // Pose
        NavigatorPose pose;
        std::string currentStation;
        std::string lastStation;
        std::string targetId;
        
        std::vector<std::string> unfinished_path;
        std::vector<std::string> finished_path;

        // Velocity (actual)
        double      vx        = 0;  // m/s
        double      vy        = 0;  // m/s
        double      w         = 0;  // rad/s
        bool        isStopped = false;

        // Obstacles
        NavigatorObstacleInfo blocked;
        NavigatorObstacleInfo slowed;

        // Battery
        NavigatorBatteryInfo battery;

        // Task / navigation
        NavigatorTaskState  taskStatus = NavigatorTaskState::None;

        // System
        NavigatorRelocationState locStatus = NavigatorRelocationState::Unknown;
        bool emergency = false;
        std::string currentMap;

        std::unordered_map<std::string,std::string> errors; // {"error_code":"desc_error"}
        std::unordered_map<std::string,std::string> fatals; // {"fatal_code":"desc_fatal"}

        std::string state_raw;
    };
}