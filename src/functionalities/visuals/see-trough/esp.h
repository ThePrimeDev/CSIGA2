#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <mutex>
#include <imgui.h>

#include "memory/memory.h"

namespace esp {

// ESP settings
struct EspSettings {
    bool show_box = true;
    bool show_skeleton = true;
    bool show_health = true;
    bool show_name = true;
    bool show_weapon = true;
    bool show_head = true;
    bool enemies_only = true;

    ImVec4 enemy_color = ImVec4(1.0f, 0.55f, 0.0f, 1.0f);
    ImVec4 friendly_color = ImVec4(0.0f, 0.8f, 0.2f, 1.0f);
    ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);
};

// Data for rendering a single player
struct EspPlayerData {
    memory::Vec3 position;
    memory::Vec3 head_position;
    std::unordered_map<memory::Bones, memory::Vec3> bones;
    std::string name;
    int32_t health;
    int32_t armor;
    bool is_enemy;
    bool visible;
    memory::Weapon weapon;
    int32_t ping = 0;
    float rotation;
    uint64_t pawn = 0;
};

// Grenade proximity warning data
struct GrenadeProximityData {
    memory::Vec3 position;
    float radius;                       // danger radius in game units
    std::vector<memory::Vec3> fire_hull; // fire positions (Inferno only)
    enum Type { HE, Flash, Smoke, Molotov, Decoy, Inferno } type;
};

// Main ESP manager - handles memory connection and data caching
class EspManager {
public:
    EspManager();
    ~EspManager();

    // Try to connect to the game
    bool connect();

    // Disconnect from game
    void disconnect();

    // Check if connected
    bool isConnected() const;

    // Update game data (call once per frame)
    void update();

    // Get current settings
    EspSettings& settings() { return settings_; }
    const EspSettings& settings() const { return settings_; }

    // Get cached player data for rendering
    const std::vector<EspPlayerData>& getPlayers() const { return players_; }
    const std::vector<GrenadeProximityData>& getGrenadeProximity() const { return grenadeProximity_; }

    // Get local player team
    uint8_t getLocalTeam() const { return localTeam_; }

    // Get view matrix for world-to-screen
    const memory::Mat4& getViewMatrix() const { return viewMatrix_; }

    // Get window size
    memory::IVec2 getWindowSize() const { return windowSize_; }

    // Get local player weapon name
    const std::string& getLocalWeapon() const { return localWeapon_; }

    // Get cached bomb data
    const memory::BombData& getBombData() const { return bomb_; }

    // Check if local player is scoped
    bool isLocalPlayerScoped() const { return localIsScoped_; }

    int32_t getLocalMvpCount() const { return localMvpCount_; }

    // Check if local player is using a sniper
    bool isLocalPlayerHoldingSniper() const;

    // Local player view data
    memory::Vec2 getLocalViewAngles() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return localViewAngles_;
    }

    float getLocalFov() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return localFov_;
    }

    // Accessors for other modules (e.g., Clickity)
    const std::optional<memory::Process>& getProcess() const { return process_; }
    const std::optional<memory::Offsets>& getOffsets() const { return offsets_; }

private:
    void cachePlayerData();

    mutable std::mutex mutex_;
    std::optional<memory::Process> process_;
    std::optional<memory::Offsets> offsets_;

    EspSettings settings_;
    std::vector<EspPlayerData> players_;
    uint8_t localTeam_ = 0;
    memory::Mat4 viewMatrix_;
    memory::IVec2 windowSize_;
    uint64_t localPawnIndex_ = 0;
    std::string localWeapon_;
    bool localIsScoped_ = false;
    int32_t localMvpCount_ = 0;
    memory::Vec2 localViewAngles_{};
    float localFov_ = static_cast<float>(memory::cs2::DEFAULT_FOV);
    uint64_t planted_c4_handle_ = 0;
    float last_c4_scan_time_ = 0.0f;
    memory::BombData bomb_;
    std::vector<GrenadeProximityData> grenadeProximity_;
};

// Global ESP manager instance
extern EspManager g_esp_manager;

// Rendering functions
void RenderEsp(const ImVec2& window_pos, 
               const ImVec2& window_size, 
               const std::vector<EspPlayerData>& players,
               const memory::Mat4& view_matrix);

// World to screen projection
std::optional<ImVec2> WorldToScreen(const memory::Vec3& world_pos,
                                     const memory::Mat4& view_matrix,
                                     const ImVec2& screen_size);

} // namespace esp
