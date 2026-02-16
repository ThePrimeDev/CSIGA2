#include "clickity.h"
#include "../../platform/input.h"
#include "../../memory/process.h"
#include "../../memory/find_offsets.h"
#include "../../memory/entity/player.h" // For Player class
#include "../../functionalities/visuals/see-trough/esp.h" // To reuse the EspManager's process/offsets if possible, or we define our own access

#include <thread>
#include <iostream>

namespace functionalities {

// Use the global ESP manager to get access to process and memory.
// This avoids duplicating the connection logic.
// In a cleaner architecture, we might have a central "GameContext" class.
// For now, we'll access the global singleton from esp.h 

Clickity& Clickity::Get() {
    static Clickity instance;
    return instance;
}

Clickity::Clickity() : rng_(std::random_device{}()) {}

void Clickity::Update() {
    ReadGameData();
    ApplyAction();
}

void Clickity::ReadGameData() {
    std::lock_guard<std::mutex> lock(mutex_);
    should_schedule_shot_ = false;

    if (!enabled_) {
        shot_start_.reset();
        shot_end_.reset();
        cooldown_end_.reset();
        return;
    }

    auto now = std::chrono::steady_clock::now();

    // Don't compute new targets if we're mid-shot or cooling down
    if (shot_start_.has_value() || shot_end_.has_value()) {
        return;
    }

    if (cooldown_end_.has_value()) {
        if (now < cooldown_end_.value()) {
            return;
        }
        cooldown_end_.reset();
    }

    // Basic connection check
    if (!esp::g_esp_manager.isConnected()) {
        return;
    }

    // We need to access memory. The ESP manager has the process and offsets.
    // However, the ESP manager's accessors are not fully exposed for raw process access.
    // We will need to make `process_` and `offsets_` public or friend in EspManager, 
    // or add a getter. 
    // Looking at esp.h (implied), we can probably add getters there or use what's available.
    // Since I cannot easily modify esp.h to add getters right now without seeing it again,
    // I made a small assumption. Let's check esp.h content again if needed.
    // Actually, I can modify esp.h to expose getProcess() and getOffsets().
    // But for now, let's assume we can add them or they exist.
    // Wait, the user didn't ask me to modify esp.h, but I can.
    
    // Actually, looking at the provided code for esp.cpp, `process_` is private.
    // I should add getters to `EspManager` in `esp.h` to make this clean.
    
    // For this step, I will implement the logic assuming I have access.
    // I will add the getters in a separate tool call.
    
    // Access via the global instance
    const auto& processOpt = esp::g_esp_manager.getProcess();
    const auto& offsetsOpt = esp::g_esp_manager.getOffsets(); // I will need to add this method

    if (!processOpt.has_value() || !offsetsOpt.has_value()) return;

    const auto& process = processOpt.value();
    const auto& offsets = offsetsOpt.value();

    auto localPlayerOpt = memory::Player::localPlayer(process, offsets);
    if (!localPlayerOpt.has_value()) return;

    const auto& localPlayer = localPlayerOpt.value();

    // Checks
    if (flash_check_ && localPlayer.isFlashed(process, offsets)) return;

    if (scope_check_) {
        // Simple check: if weapon is sniper, must be scoped.
        // We need to know if it is a sniper.
        // The Rust code checks `weapon_class`. 
        // We have `weaponClass` in Player.
        auto wpnClass = localPlayer.weaponClass(process, offsets);
        if (wpnClass == memory::WeaponClass::Sniper) {
            if (!localPlayer.isScoped(process, offsets)) return;
        }
    }

    if (velocity_check_) {
        auto vel = localPlayer.velocity(process, offsets);
        // Length of velocity vector (x, y, z)
        float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
        if (speed > velocity_threshold_) return;
    }

    // Crosshair ID check
    auto targetOpt = localPlayer.crosshairEntity(process, offsets);
    if (!targetOpt.has_value()) return;
    
    const auto& target = targetOpt.value();
    
    // Team check - always active for now unless we add a specific toggle for "Teammates"
    // Rust logic: if !is_ffa && same team -> return
    // We assume standard checks for now.
    if (team_check_) {
        if (localPlayer.team(process, offsets) == target.team(process, offsets)) return;
    }
    
    // We have a valid target
    should_schedule_shot_ = true;
}

void Clickity::ApplyAction() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!enabled_) return;

    auto now = std::chrono::steady_clock::now();
    auto mouse = platform::Mouse::Get();

    // Handle shooting state machine
    if (shot_start_.has_value() && now >= shot_start_.value()) {
        mouse->LeftPress();
        shot_start_.reset();
    }

    if (shot_end_.has_value() && now >= shot_end_.value()) {
        mouse->LeftRelease();
        shot_end_.reset();
        cooldown_end_ = now + std::chrono::milliseconds(click_delay_);
    }

    // Schedule new shot if ReadGameData found a target
    if (should_schedule_shot_ && !shot_start_.has_value() && !shot_end_.has_value()) {
        should_schedule_shot_ = false;

        // We have a target! Calculate delay.
        float mean = (delay_start_ + delay_end_) / 2.0f;
        float std_dev = (delay_end_ - delay_start_) / 2.0f;
        if (std_dev < 0.1f) std_dev = 0.1f; // Avoid zero division/invalid std_dev
        
        std::normal_distribution<float> d(mean, std_dev);
        float delay_ms = d(rng_);
        if (delay_ms < 0.0f) delay_ms = 0.0f;
        
        shot_start_ = now + std::chrono::milliseconds(static_cast<long long>(delay_ms));
        shot_end_ = shot_start_.value() + std::chrono::milliseconds(shot_duration_);
    }
}

} // namespace functionalities
