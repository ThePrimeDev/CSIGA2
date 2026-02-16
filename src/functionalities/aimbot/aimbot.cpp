#include "aimbot.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>

#include "../../functionalities/visuals/see-trough/esp.h"
#include "../../memory/memory.h"
#include "../../platform/input.h"
#include "../../ui/state.h"

#include <linux/input-event-codes.h>

namespace functionalities {

namespace {

// Map AimbotKey enum to Linux evdev key code
int AimbotKeyToEvdev(uint64_t key) {
    using namespace functionalities;
    switch (static_cast<AimbotKey>(key)) {
        case AimbotKey::Mouse4:       return BTN_SIDE;
        case AimbotKey::Mouse5:       return BTN_EXTRA;
        case AimbotKey::MouseLeft:    return BTN_LEFT;
        case AimbotKey::MouseRight:   return BTN_RIGHT;
        case AimbotKey::MouseMiddle:  return BTN_MIDDLE;
        case AimbotKey::Space:        return KEY_SPACE;
        case AimbotKey::C:            return KEY_C;
        case AimbotKey::V:            return KEY_V;
        case AimbotKey::X:            return KEY_X;
        case AimbotKey::Z:            return KEY_Z;
        case AimbotKey::F:            return KEY_F;
        case AimbotKey::E:            return KEY_E;
        case AimbotKey::Q:            return KEY_Q;
        default:                      return -1;
    }
}

bool IsButtonDown(uint64_t key) {
    int evdev_code = AimbotKeyToEvdev(key);
    if (evdev_code < 0) return false;

    auto& reader = platform::InputReader::Get();
    // Mouse buttons use BTN_* codes (>= 0x100)
    if (evdev_code >= BTN_MISC) {
        return reader.IsMouseButtonDown(evdev_code);
    }
    return reader.IsKeyDown(evdev_code);
}

// Source engine default m_yaw and m_pitch
constexpr float kMYaw = 0.022f;
constexpr float kMPitch = 0.022f;

bool IsFfa(const memory::Process& process, const memory::Offsets& offsets) {
    if (offsets.convar.ffa == 0) {
        return false;
    }
    return process.read<uint8_t>(offsets.convar.ffa + 0x58) == 1;
}

float DistanceScale(float distance) {
    if (distance > 500.0f) {
        return 1.0f;
    }
    return 5.0f - (distance / 125.0f);
}

memory::Vec2 AngleToTarget(const memory::Player& local_player,
                           const memory::Process& process,
                           const memory::Offsets& offsets,
                           const memory::Vec3& position,
                           const memory::Vec2& aim_punch) {
    memory::Vec3 eye_position = local_player.eyePosition(process, offsets);
    memory::Vec3 forward = (position - eye_position).normalize();

    memory::Vec2 angles = memory::anglesFromVector(forward) - aim_punch;
    memory::vec2Clamp(angles);
    return angles;
}

struct TargetInfo {
    memory::Player player;
    memory::Vec2 angle;
    float distance = 0.0f;
    float fov = 0.0f;
    bool valid = false;
};

TargetInfo FindTarget(const memory::Process& process,
                      const memory::Offsets& offsets,
                      const memory::Player& local_player,
                      const memory::Vec2& view_angles,
                      const memory::Vec2& aim_punch,
                      float max_fov,
                      bool distance_adjusted_fov,
                      bool target_friendlies,
                      bool visibility_check,
                      bool head_only) {
    TargetInfo best;
    float best_fov = 360.0f;

    bool ffa = IsFfa(process, offsets);
    uint8_t local_team = local_player.team(process, offsets);
    int64_t local_spotted_mask = local_player.spottedMask(process, offsets);
    memory::Vec3 eye_position = local_player.eyePosition(process, offsets);

    constexpr size_t kNumBuckets = 64;
    auto bucket_pointers = process.readVec(offsets.interface_.entity, 0x8 * kNumBuckets);

    for (size_t bucket_index = 0; bucket_index < kNumBuckets; ++bucket_index) {
        uint64_t bucket_pointer;
        std::memcpy(&bucket_pointer, &bucket_pointers[bucket_index * 8], sizeof(uint64_t));
        if (bucket_pointer == 0 || (bucket_pointer >> 48) != 0) {
            continue;
        }

        constexpr size_t kIdentitiesPerBucket = 512;
        auto bucket = process.readVec(bucket_pointer, kIdentitiesPerBucket * offsets.entity_identity.size);

        for (size_t index_in_bucket = 0; index_in_bucket < kIdentitiesPerBucket; ++index_in_bucket) {
            size_t identity_offset = index_in_bucket * offsets.entity_identity.size;

            uint64_t entity;
            std::memcpy(&entity, &bucket[identity_offset], sizeof(uint64_t));
            if (entity == 0) {
                continue;
            }

            size_t handle_start = identity_offset + 0x10;
            uint32_t handle;
            std::memcpy(&handle, &bucket[handle_start], sizeof(uint32_t));
            uint32_t handle_index = handle & 0x7FFF;
            uint32_t entity_index = static_cast<uint32_t>(bucket_index * kIdentitiesPerBucket + index_in_bucket);
            if (entity_index != handle_index) {
                continue;
            }

            uint64_t vtable = process.read<uint64_t>(entity);
            uint64_t rtti = process.read<uint64_t>(vtable - 0x8);
            uint64_t name_ptr = process.read<uint64_t>(rtti + 0x8);
            std::string name = process.readStringUncached(name_ptr);
            if (name != memory::cs2::entity_class::PLAYER_CONTROLLER) {
                continue;
            }

            auto player_opt = memory::Player::fromController(entity, process, offsets);
            if (!player_opt.has_value()) {
                continue;
            }

            memory::Player player = player_opt.value();
            if (!player.isValid(process, offsets)) {
                continue;
            }
            if (player == local_player) {
                continue;
            }

            if (!target_friendlies && !ffa && local_team == player.team(process, offsets)) {
                continue;
            }

            if (visibility_check) {
                uint64_t raw_index = static_cast<uint64_t>(handle) & 0x7FFF;
                if (raw_index == 0) {
                    continue;
                }
                uint64_t pawn_index = raw_index - 1;
                bool visible = (local_spotted_mask & (1LL << pawn_index)) != 0;
                if (!visible) {
                    continue;
                }
            }

            // Find the best bone to aim at
            memory::Vec3 best_bone_pos;
            memory::Vec2 best_bone_angle;
            float best_bone_fov = std::numeric_limits<float>::max();

            if (head_only) {
                best_bone_pos = player.bonePosition(process, offsets, static_cast<uint64_t>(memory::Bones::Head));
                best_bone_angle = AngleToTarget(local_player, process, offsets, best_bone_pos, aim_punch);
                best_bone_fov = memory::anglesToFov(view_angles, best_bone_angle);
            } else {
                constexpr std::array<memory::Bones, 7> kAimBones = {
                    memory::Bones::Head,
                    memory::Bones::Neck,
                    memory::Bones::Spine4,
                    memory::Bones::Spine3,
                    memory::Bones::Spine2,
                    memory::Bones::Spine1,
                    memory::Bones::Hip,
                };

                for (memory::Bones bone : kAimBones) {
                    memory::Vec3 bone_pos = player.bonePosition(process, offsets, static_cast<uint64_t>(bone));
                    memory::Vec2 angle = AngleToTarget(local_player, process, offsets, bone_pos, aim_punch);
                    float fov = memory::anglesToFov(view_angles, angle);
                    if (fov < best_bone_fov) {
                        best_bone_fov = fov;
                        best_bone_pos = bone_pos;
                        best_bone_angle = angle;
                    }
                }
            }

            float distance = eye_position.distance(best_bone_pos);

            float fov_limit = max_fov;
            if (distance_adjusted_fov) {
                fov_limit *= DistanceScale(distance);
            }

            if (best_bone_fov > fov_limit) {
                continue;
            }

            if (best_bone_fov < best_fov) {
                best_fov = best_bone_fov;
                best.player = player;
                best.angle = best_bone_angle;
                best.distance = distance;
                best.fov = best_bone_fov;
                best.valid = true;
            }
        }
    }

    return best;
}

} // namespace

Aimbot& Aimbot::Get() {
    static Aimbot instance;
    return instance;
}

void Aimbot::Update() {
    ReadGameData();
    ApplyAim();
}

void Aimbot::ReadGameData() {
    std::lock_guard<std::mutex> lock(mutex_);
    has_aim_delta_ = false;
    aim_delta_x_ = 0;
    aim_delta_y_ = 0;

    if (!g_aimbot_enabled) {
        previous_button_state_ = false;
        active_ = false;
        return;
    }

    if (!esp::g_esp_manager.isConnected()) {
        return;
    }

    const auto& process_opt = esp::g_esp_manager.getProcess();
    const auto& offsets_opt = esp::g_esp_manager.getOffsets();
    if (!process_opt.has_value() || !offsets_opt.has_value()) {
        return;
    }

    const auto& process = process_opt.value();
    const auto& offsets = offsets_opt.value();

    auto local_player_opt = memory::Player::localPlayer(process, offsets);
    if (!local_player_opt.has_value()) {
        return;
    }

    const auto& local_player = local_player_opt.value();


    bool button_state = IsButtonDown(static_cast<uint64_t>(g_aimbot_hotkey));
    if (g_aimbot_key_mode == 0) {
        if (!button_state) {
            locked_pawn_ = 0;  // Release target lock when key is released
            return;
        }
    } else {
        if (button_state && !previous_button_state_) {
            active_ = !active_;
            if (!active_) locked_pawn_ = 0;
        }
        previous_button_state_ = button_state;
        if (!active_) {
            return;
        }
    }

    if (g_aimbot_flash_check && local_player.isFlashed(process, offsets)) {
        return;
    }

    memory::Vec2 aim_punch = local_player.aimPunch(process, offsets) * 2.0f;
    if (local_player.weaponClass(process, offsets) == memory::WeaponClass::Sniper) {
        aim_punch = memory::Vec2::ZERO;
    } else if (aim_punch.length() == 0.0f && local_player.shotsFired(process, offsets) > 1) {
        aim_punch = memory::Vec2(previous_aim_punch_x_, previous_aim_punch_y_);
    }
    previous_aim_punch_x_ = aim_punch.x;
    previous_aim_punch_y_ = aim_punch.y;

    memory::Vec2 view_angles = local_player.viewAngles(process, offsets);

    TargetInfo target = FindTarget(process,
                                   offsets,
                                   local_player,
                                   view_angles,
                                   aim_punch,
                                   g_aimbot_fov,
                                   g_aimbot_distance_adjusted_fov,
                                   g_aimbot_target_friendlies,
                                   g_aimbot_visibility_check,
                                   g_aimbot_head_only);

    if (!target.valid) {
        locked_pawn_ = 0;
        return;
    }

    if (!target.player.isValid(process, offsets)) {
        locked_pawn_ = 0;
        return;
    }

    // Target locking: if we have a locked target, keep it unless it's invalid
    if (locked_pawn_ != 0 && locked_pawn_ != target.player.getPawn()) {
        // Check if locked target is still valid
        memory::Player locked_player(0, locked_pawn_);
        if (locked_player.isValid(process, offsets)) {
            // Re-calculate angle to locked target
            memory::Vec3 head_pos = locked_player.bonePosition(process, offsets, static_cast<uint64_t>(memory::Bones::Head));
            memory::Vec2 locked_angle = AngleToTarget(local_player, process, offsets, head_pos, aim_punch);
            float locked_fov = memory::anglesToFov(view_angles, locked_angle);
            float fov_limit = g_aimbot_fov;
            if (g_aimbot_distance_adjusted_fov) {
                fov_limit *= DistanceScale(local_player.eyePosition(process, offsets).distance(head_pos));
            }
            if (locked_fov <= fov_limit * 2.0f) {  // Allow wider FOV for locked target
                target.angle = locked_angle;
                target.player = locked_player;
            } else {
                locked_pawn_ = target.player.getPawn();
            }
        } else {
            locked_pawn_ = target.player.getPawn();
        }
    } else {
        locked_pawn_ = target.player.getPawn();
    }

    if (local_player.shotsFired(process, offsets) < g_aimbot_start_bullet) {
        return;
    }

    memory::Vec2 target_angle = target.angle;


    // === Compute mouse delta ===
    memory::Vec2 aim_angles = view_angles - target_angle;
    if (aim_angles.y < -180.0f) {
        aim_angles.y += 360.0f;
    }
    if (aim_angles.y > 180.0f) {
        aim_angles.y -= 360.0f;
    }
    memory::vec2Clamp(aim_angles);

    float sensitivity = process.read<float>(offsets.convar.sensitivity + 0x58) *
                        local_player.fovMultiplier(process, offsets);
    if (sensitivity <= 0.0f) {
        return;
    }

    float smooth = std::clamp(g_aimbot_smooth + 1.0f, 1.0f, 20.0f);

    // Convert angle delta to mouse counts using m_yaw/m_pitch
    float mouse_x = aim_angles.y / (sensitivity * kMYaw);
    float mouse_y = -aim_angles.x / (sensitivity * kMPitch);

    // Apply smoothing
    mouse_x /= smooth;
    mouse_y /= smooth;

    // Accumulate fractional remainder to prevent precision loss
    mouse_x += mouse_remainder_x_;
    mouse_y += mouse_remainder_y_;

    int32_t int_x = static_cast<int32_t>(mouse_x);
    int32_t int_y = static_cast<int32_t>(mouse_y);

    mouse_remainder_x_ = mouse_x - static_cast<float>(int_x);
    mouse_remainder_y_ = mouse_y - static_cast<float>(int_y);

    if (int_x != 0 || int_y != 0) {
        has_aim_delta_ = true;
        aim_delta_x_ = int_x;
        aim_delta_y_ = int_y;
    }
}

void Aimbot::ApplyAim() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_aim_delta_) return;
    has_aim_delta_ = false;

    auto mouse = platform::Mouse::Get();
    if (mouse && mouse->IsValid()) {
        platform::Vec2 move{static_cast<float>(aim_delta_x_), static_cast<float>(aim_delta_y_)};
        mouse->MoveRel(move);
    }
}

} // namespace functionalities
