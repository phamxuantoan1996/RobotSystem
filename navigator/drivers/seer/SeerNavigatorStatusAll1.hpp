#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace navigator::drivers::seer {
    struct SeerStatusAll1 {
        // -- Pose
        double x = 0; // m
        double y = 0; // m
        double angle = 0; // rad
        double confidence = 0; // [min,max] = [0,1]

        // station
        std::string current_station;
        std::string last_station;
        std::string target_id;
        std::vector<std::string> unfinished_path;
        std::vector<std::string> finished_path;

        // velocity
        double vx = 0;
        double vy = 0;
        double w = 0;
        bool is_stop = false;

        // obstacle: blocked
        bool blocked = false;
        int block_reason = 0;

        double block_x = 0;
        double block_y = 0;

        // obstacle: slowed
        bool slowed = false;
        int slow_reason = 0;
        double slow_x = 0;
        double slow_y = 0;

        // battery
        double battery_level = 0; // [min,max] = [0,1]
        double voltage = 0;
        double current = 0;
        double charging = false;

        // task navigation
        int task_status; // 0 = None, 1 = WAITING, 2 = RUNNING, 3 = SUSPENDED, 4 = COMPLETED, 5 = FAILED, 6 = CANCELED

        // relocation
        int reloc_status = 0; // 0 = RELOC_INIT, 1 = RELOC_SUCCESS, 2 = RELOC_RELOCING

        // emergency
        bool emergency = false;
        bool driver_emc = false;
        
        // errors, fatals
        std::unordered_map<int,std::string> errors;
        std::unordered_map<int,std::string> fatals;

        // current map
        std::string current_map;

        // state raw
        std::string state_raw;
    };

}