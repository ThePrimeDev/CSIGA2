#pragma once

#include <chrono>
#include <optional>
#include <random>

namespace functionalities {

class Clickity {
public:
    Clickity();
    ~Clickity() = default;

    // Singleton access
    static Clickity& Get();

    // Main update loop
    void Update();

    // Configuration
    bool enabled_ = false;
    bool team_check_ = true;
    bool flash_check_ = true;
    bool scope_check_ = true;
    bool velocity_check_ = true; // Not strictly used in Rust logic for shooting, but good to have
    float velocity_threshold_ = 5.0f; 
    
    // Delay settings (in milliseconds)
    int delay_start_ = 10;
    int delay_end_ = 30;
    int shot_duration_ = 50; // How long to hold the button
    int click_delay_ = 80; // Delay between shots

private:
    std::optional<std::chrono::steady_clock::time_point> shot_start_;
    std::optional<std::chrono::steady_clock::time_point> shot_end_;
    std::optional<std::chrono::steady_clock::time_point> cooldown_end_;
    
    bool previous_button_state_ = false;
    
    // Random number generation for delay
    std::mt19937 rng_;
};

} // namespace functionalities
