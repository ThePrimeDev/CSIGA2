#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <random>

namespace functionalities {

class Clickity {
public:
    Clickity();
    ~Clickity() = default;

    // Singleton access
    static Clickity& Get();

    // Legacy single-threaded update (calls both phases)
    void Update();

    // Phase 1: Read game memory and decide if we should shoot (background thread)
    void ReadGameData();
    // Phase 2: Apply cached shot timing (background thread or render thread)
    void ApplyAction();

    // Configuration
    bool enabled_ = false;
    bool team_check_ = true;
    bool flash_check_ = true;
    bool scope_check_ = true;
    bool velocity_check_ = true;
    float velocity_threshold_ = 5.0f; 
    
    // Delay settings (in milliseconds)
    int delay_start_ = 10;
    int delay_end_ = 30;
    int shot_duration_ = 50;
    int click_delay_ = 80;

private:
    std::mutex mutex_;
    std::optional<std::chrono::steady_clock::time_point> shot_start_;
    std::optional<std::chrono::steady_clock::time_point> shot_end_;
    std::optional<std::chrono::steady_clock::time_point> cooldown_end_;
    
    bool previous_button_state_ = false;
    
    // Cached result from ReadGameData()
    bool should_schedule_shot_ = false;
    
    // Random number generation for delay
    std::mt19937 rng_;
};

} // namespace functionalities
