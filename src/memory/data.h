#pragma once

#include "math.h"
#include "bones.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace memory {

// Forward declaration
enum class Weapon;

enum class SoundType {
    Footstep,
    Gunshot,
    Weapon
};

struct PlayerData {
    uint64_t steam_id = 0;
    int32_t health = 0;
    int32_t armor = 0;
    Vec3 position;
    Vec3 head;
    std::string name;
    int weapon_id = 0;  // Weapon enum value
    std::unordered_map<Bones, Vec3> bones;
    bool has_defuser = false;
    bool has_helmet = false;
    bool has_bomb = false;
    bool visible = false;
    int32_t color = 0;
    float rotation = 0.0f;
    std::optional<SoundType> sound;
};

struct BombData {
    bool planted = false;
    float timer = 0.0f;
    bool being_defused = false;
    Vec3 position;
    float defuse_remain_time = 0.0f;
    int32_t bomb_site = -1;
};

struct Data {
    bool in_game = false;
    bool is_ffa = false;
    bool is_custom_mode = false;
    int weapon_id = 0;  // Weapon enum value
    std::vector<PlayerData> players;
    std::vector<PlayerData> friendlies;
    PlayerData local_player;
    // Note: entities vector would need EntityInfo type
    BombData bomb;
    std::string map_name;
    Mat4 view_matrix;
    Vec2 view_angles;
    Vec2 window_position;
    Vec2 window_size;
    bool aimbot_active = false;
    bool triggerbot_active = false;
    bool wallhack_active = false;
    std::vector<std::pair<uint64_t, uint64_t>> spectators;
    std::vector<std::pair<std::string, uint64_t>> spectator_names;
};

} // namespace memory
