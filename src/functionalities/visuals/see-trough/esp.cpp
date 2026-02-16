#include "esp.h"
#include <imgui_internal.h>
#include <cmath>
#include "ui/state.h"

namespace esp {

namespace {

void AddChamsSegment(ImDrawList* draw, const ImVec2& a, const ImVec2& b, float thickness, ImU32 color) {
    ImVec2 dir(b.x - a.x, b.y - a.y);
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.5f) return;

    dir.x /= len;
    dir.y /= len;
    ImVec2 perp(-dir.y, dir.x);
    ImVec2 offset(perp.x * thickness * 0.5f, perp.y * thickness * 0.5f);

    ImVec2 quad[4] = {
        ImVec2(a.x + offset.x, a.y + offset.y),
        ImVec2(a.x - offset.x, a.y - offset.y),
        ImVec2(b.x - offset.x, b.y - offset.y),
        ImVec2(b.x + offset.x, b.y + offset.y)
    };

    draw->AddConvexPolyFilled(quad, 4, color);
}

void RenderFakeChams(ImDrawList* draw,
                     const EspPlayerData& player,
                     const memory::Mat4& viewMatrix,
                     const ImVec2& screen_size,
                     const ImVec2& window_pos,
                     float height,
                     ImU32 color) {
    float limb_thickness = ImClamp(height * 0.08f, 3.0f, 9.0f);
    float torso_thickness = ImClamp(height * 0.16f, 6.0f, 14.0f);

    auto screenPoint = [&](memory::Bones bone) -> std::optional<ImVec2> {
        auto it = player.bones.find(bone);
        if (it == player.bones.end()) return std::nullopt;
        auto screen = WorldToScreen(it->second, viewMatrix, screen_size);
        if (!screen.has_value()) return std::nullopt;
        return ImVec2(screen->x + window_pos.x, screen->y + window_pos.y);
    };

    auto head = screenPoint(memory::Bones::Head);
    auto neck = screenPoint(memory::Bones::Neck);
    auto hip = screenPoint(memory::Bones::Hip);
    auto left_shoulder = screenPoint(memory::Bones::LeftShoulder);
    auto right_shoulder = screenPoint(memory::Bones::RightShoulder);
    auto left_hip = screenPoint(memory::Bones::LeftHip);
    auto right_hip = screenPoint(memory::Bones::RightHip);

    if (left_shoulder.has_value() && right_shoulder.has_value() &&
        left_hip.has_value() && right_hip.has_value()) {
        ImVec2 torso[4] = {
            left_shoulder.value(),
            right_shoulder.value(),
            right_hip.value(),
            left_hip.value()
        };
        draw->AddConvexPolyFilled(torso, 4, color);
    } else if (neck.has_value() && hip.has_value()) {
        AddChamsSegment(draw, neck.value(), hip.value(), torso_thickness, color);
    }

    if (head.has_value()) {
        float head_radius = ImClamp(height * 0.08f, 4.0f, 12.0f);
        draw->AddCircleFilled(head.value(), head_radius, color, 20);
    }

    for (const auto& connection : memory::BONE_CONNECTIONS) {
        auto it1 = player.bones.find(connection.first);
        auto it2 = player.bones.find(connection.second);

        if (it1 == player.bones.end() || it2 == player.bones.end()) continue;

        auto screen1 = WorldToScreen(it1->second, viewMatrix, screen_size);
        auto screen2 = WorldToScreen(it2->second, viewMatrix, screen_size);
        if (!screen1.has_value() || !screen2.has_value()) continue;

        ImVec2 p1(screen1->x + window_pos.x, screen1->y + window_pos.y);
        ImVec2 p2(screen2->x + window_pos.x, screen2->y + window_pos.y);

        AddChamsSegment(draw, p1, p2, limb_thickness, color);
    }
}

} // namespace

// Global instance
EspManager g_esp_manager;

static uint64_t FindPlantedC4Handle(const memory::Process& process, const memory::Offsets& offsets) {
    constexpr size_t NUM_BUCKETS = 64;
    auto bucketPointers = process.readVec(offsets.interface_.entity, 0x8 * NUM_BUCKETS);

    for (size_t bucketIndex = 0; bucketIndex < NUM_BUCKETS; ++bucketIndex) {
        uint64_t bucketPointer;
        std::memcpy(&bucketPointer, &bucketPointers[bucketIndex * 8], sizeof(uint64_t));
        if (bucketPointer == 0 || (bucketPointer >> 48) != 0) continue;

        constexpr size_t IDENTITIES_PER_BUCKET = 512;
        auto bucket = process.readVec(bucketPointer, IDENTITIES_PER_BUCKET * offsets.entity_identity.size);

        for (size_t indexInBucket = 0; indexInBucket < IDENTITIES_PER_BUCKET; ++indexInBucket) {
            size_t identityOffset = indexInBucket * offsets.entity_identity.size;

            uint64_t entity;
            std::memcpy(&entity, &bucket[identityOffset], sizeof(uint64_t));
            if (entity == 0) continue;

            size_t handleStart = identityOffset + 0x10;
            uint32_t handle;
            std::memcpy(&handle, &bucket[handleStart], sizeof(uint32_t));
            uint32_t handleIndex = handle & 0x7FFF;
            uint32_t entityIndex = static_cast<uint32_t>(bucketIndex * IDENTITIES_PER_BUCKET + indexInBucket);
            if (entityIndex != handleIndex) continue;

            uint64_t vtable = process.read<uint64_t>(entity);
            uint64_t rtti = process.read<uint64_t>(vtable - 0x8);
            uint64_t namePtr = process.read<uint64_t>(rtti + 0x8);
            std::string name = process.readStringUncached(namePtr);

            if (name == memory::cs2::entity_class::PLANTED_C4) {
                return entity;
            }
        }
    }

    return 0;
}



EspManager::EspManager() = default;
EspManager::~EspManager() = default;

bool EspManager::connect() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (process_.has_value() && process_->isValid()) {
        return true;  // Already connected
    }

    // Try to open CS2 process
    auto proc = memory::Process::open(memory::cs2::PROCESS_NAME);
    if (!proc.has_value()) {
        printf("[ESP] Failed to find CS2 process\n");
        return false;
    }
    printf("[ESP] Found CS2 process, PID: %d\n", proc->getPid());

    // Find offsets
    auto offs = memory::findOffsets(proc.value());
    if (!offs.has_value()) {
        printf("[ESP] Failed to find offsets\n");
        return false;
    }
    printf("[ESP] Found offsets successfully\n");

    process_ = std::move(proc);
    offsets_ = std::move(offs);


    return true;
}

void EspManager::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    process_.reset();
    offsets_.reset();
    players_.clear();
    g_cs2_ping = 0;
}

bool EspManager::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return process_.has_value() && process_->isValid();
}

void EspManager::update() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!process_.has_value() || !offsets_.has_value()) {
        g_cs2_ping = 0;
        return;
    }

    if (!process_->isValid()) {
        process_.reset();
        offsets_.reset();
        players_.clear();
        g_cs2_ping = 0;
        return;
    }

    const auto& process = process_.value();
    const auto& offsets = offsets_.value();

    // Get local player
    auto localPlayerOpt = memory::Player::localPlayer(process, offsets);
    if (!localPlayerOpt.has_value()) {
        players_.clear();
        g_cs2_ping = 0;
        return;
    }

    const auto& localPlayer = localPlayerOpt.value();
    localTeam_ = localPlayer.team(process, offsets);
    localMvpCount_ = localPlayer.mvpCount(process, offsets);
    {
        std::string local_name = localPlayer.name(process, offsets);
        if (!local_name.empty()) {
            g_player_name = std::move(local_name);
        }
    }
    int32_t local_ping = localPlayer.ping(process, offsets);
    g_cs2_ping = local_ping > 0 ? local_ping : 0;

    // Read local player weapon and scoped state for crosshair
    auto weapon = localPlayer.weapon(process, offsets);
    localWeapon_ = memory::weaponToString(weapon);
    localIsScoped_ = localPlayer.isScoped(process, offsets);
    localViewAngles_ = localPlayer.viewAngles(process, offsets);

    uint32_t fov_value = memory::cs2::DEFAULT_FOV;
    uint64_t camera_service = process.read<uint64_t>(localPlayer.getPawn() + offsets.pawn.camera_services);
    if (camera_service != 0) {
        uint32_t camera_fov = process.read<uint32_t>(camera_service + offsets.camera_services.fov);
        if (camera_fov > 0) {
            fov_value = camera_fov;
        }
    }
    if (fov_value == 0 && localPlayer.getController() != 0) {
        uint32_t desired_fov = process.read<uint32_t>(localPlayer.getController() + offsets.controller.desired_fov);
        if (desired_fov > 0) {
            fov_value = desired_fov;
        }
    }
    localFov_ = static_cast<float>(fov_value);

    float current_time = 0.0f;
    uint64_t global_vars = process.read<uint64_t>(offsets.direct.global_vars);
    if (global_vars != 0) {
        current_time = process.read<float>(global_vars + 0x30);
    }

    if (g_see_through_walls || g_grenade_pred || g_grenade_proximity_warning) {
        // Read view matrix
        viewMatrix_ = process.read<memory::Mat4>(offsets.direct.view_matrix);

        // Read window size from SDL window
        uint64_t sdlWindow = process.read<uint64_t>(offsets.direct.sdl_window);
        if (sdlWindow != 0) {
            windowSize_.x = process.read<int32_t>(sdlWindow + 0x18);
            windowSize_.y = process.read<int32_t>(sdlWindow + 0x1C);
        }

        // Cache player data
        if (g_see_through_walls || g_grenade_proximity_warning) {
            cachePlayerData();
        }
    }

    if (g_where_is_planted) {
        if (!g_see_through_walls) {
            float ui_time = static_cast<float>(ImGui::GetTime());
            bool need_scan = (planted_c4_handle_ == 0) || (ui_time - last_c4_scan_time_ > 0.75f);
            if (need_scan) {
                last_c4_scan_time_ = ui_time;
                planted_c4_handle_ = FindPlantedC4Handle(process, offsets);
            }
        }

        if (planted_c4_handle_ != 0) {
            memory::PlantedC4 c4(planted_c4_handle_);
            if (c4.isRelevant(process, offsets) && c4.isPlanted(process, offsets)) {
                bomb_.planted = true;
                bomb_.timer = c4.timeToExplosion(process, offsets, current_time);
                bomb_.position = c4.position(process, offsets);
                bomb_.being_defused = c4.isBeingDefused(process, offsets);
                bomb_.defuse_remain_time = c4.timeToDefuse(process, offsets, current_time);
                bomb_.bomb_site = c4.bombSite(process, offsets);

                if (bomb_.timer < 0.0f) bomb_.timer = 0.0f;
                if (bomb_.defuse_remain_time < 0.0f) bomb_.defuse_remain_time = 0.0f;
            } else {
                bomb_.planted = false;
                bomb_.timer = 0.0f;
                bomb_.position = memory::Vec3{};
                bomb_.being_defused = false;
                bomb_.defuse_remain_time = 0.0f;
                bomb_.bomb_site = -1;
                if (!g_see_through_walls) {
                    planted_c4_handle_ = 0;
                }
            }
        } else {
            bomb_.planted = false;
            bomb_.timer = 0.0f;
            bomb_.position = memory::Vec3{};
            bomb_.being_defused = false;
            bomb_.defuse_remain_time = 0.0f;
            bomb_.bomb_site = -1;
        }
    } else {
        bomb_.planted = false;
        bomb_.timer = 0.0f;
        bomb_.position = memory::Vec3{};
        bomb_.being_defused = false;
        bomb_.defuse_remain_time = 0.0f;
        bomb_.bomb_site = -1;
        planted_c4_handle_ = 0;
    }
}

void EspManager::cachePlayerData() {
    players_.clear();
    grenadeProximity_.clear();

    if (!process_.has_value() || !offsets_.has_value()) {
        return;
    }

    const auto& process = process_.value();
    const auto& offsets = offsets_.value();

    // Get local player for comparison
    auto localPlayerOpt = memory::Player::localPlayer(process, offsets);
    if (!localPlayerOpt.has_value()) return;

    const auto& localPlayer = localPlayerOpt.value();
    int64_t localSpottedMask = localPlayer.spottedMask(process, offsets);
    std::optional<uint64_t> spectated_pawn;
    if (!localPlayer.isValid(process, offsets)) {
        auto spectated = localPlayer.spectatorTarget(process, offsets);
        if (spectated.has_value()) {
            spectated_pawn = spectated->getPawn();
        }
    }

    if (g_where_is_planted) {
        planted_c4_handle_ = 0;
    }

    // Iterate through entity buckets to find players
    constexpr size_t NUM_BUCKETS = 64;
    auto bucketPointers = process.readVec(offsets.interface_.entity, 0x8 * NUM_BUCKETS);

    for (size_t bucketIndex = 0; bucketIndex < 64; ++bucketIndex) {
        uint64_t bucketPointer;
        std::memcpy(&bucketPointer, &bucketPointers[bucketIndex * 8], sizeof(uint64_t));
        if (bucketPointer == 0 || (bucketPointer >> 48) != 0) continue;

        constexpr size_t IDENTITIES_PER_BUCKET = 512;
        auto bucket = process.readVec(bucketPointer, IDENTITIES_PER_BUCKET * offsets.entity_identity.size);

        for (size_t indexInBucket = 0; indexInBucket < IDENTITIES_PER_BUCKET; ++indexInBucket) {
            size_t identityOffset = indexInBucket * offsets.entity_identity.size;

            uint64_t entity;
            std::memcpy(&entity, &bucket[identityOffset], sizeof(uint64_t));
            if (entity == 0) continue;

            // Check handle validity
            size_t handleStart = identityOffset + 0x10;
            uint32_t handle;
            std::memcpy(&handle, &bucket[handleStart], sizeof(uint32_t));
            uint32_t handleIndex = handle & 0x7FFF;
            uint32_t entityIndex = static_cast<uint32_t>(bucketIndex * IDENTITIES_PER_BUCKET + indexInBucket);
            if (entityIndex != handleIndex) continue;

            // Get entity class name via RTTI
            uint64_t vtable = process.read<uint64_t>(entity);
            uint64_t rtti = process.read<uint64_t>(vtable - 0x8);
            uint64_t namePtr = process.read<uint64_t>(rtti + 0x8);
            std::string name = process.readStringUncached(namePtr);

            if (g_where_is_planted && name == memory::cs2::entity_class::PLANTED_C4) {
                planted_c4_handle_ = entity;
                continue;
            }

            // Grenade proximity warning: detect grenade/fire entities
            if (g_grenade_proximity_warning) {
                if (name == memory::cs2::entity_class::INFERNO) {
                    memory::Inferno inferno(entity);
                    if (inferno.isBurning(process, offsets)) {
                        GrenadeProximityData data;
                        data.position = memory::Player::entity(entity).position(process, offsets);
                        data.radius = 200.0f;
                        data.fire_hull = inferno.hull(process, offsets);
                        data.type = GrenadeProximityData::Inferno;
                        grenadeProximity_.push_back(std::move(data));
                    }
                    continue;
                } else if (name == memory::cs2::entity_class::HE_GRENADE) {
                    GrenadeProximityData data;
                    data.position = memory::Player::entity(entity).position(process, offsets);
                    data.radius = 350.0f;  // HE grenade blast radius
                    data.type = GrenadeProximityData::HE;
                    grenadeProximity_.push_back(std::move(data));
                    continue;
                } else if (name == memory::cs2::entity_class::FLASHBANG) {
                    GrenadeProximityData data;
                    data.position = memory::Player::entity(entity).position(process, offsets);
                    data.radius = 600.0f;  // Flashbang effect radius
                    data.type = GrenadeProximityData::Flash;
                    grenadeProximity_.push_back(std::move(data));
                    continue;
                } else if (name == memory::cs2::entity_class::SMOKE) {
                    GrenadeProximityData data;
                    data.position = memory::Player::entity(entity).position(process, offsets);
                    data.radius = 144.0f;  // Smoke radius
                    data.type = GrenadeProximityData::Smoke;
                    grenadeProximity_.push_back(std::move(data));
                    continue;
                } else if (name == memory::cs2::entity_class::MOLOTOV) {
                    GrenadeProximityData data;
                    data.position = memory::Player::entity(entity).position(process, offsets);
                    data.radius = 200.0f;
                    data.type = GrenadeProximityData::Molotov;
                    grenadeProximity_.push_back(std::move(data));
                    continue;
                } else if (name == memory::cs2::entity_class::DECOY) {
                    GrenadeProximityData data;
                    data.position = memory::Player::entity(entity).position(process, offsets);
                    data.radius = 100.0f;
                    data.type = GrenadeProximityData::Decoy;
                    grenadeProximity_.push_back(std::move(data));
                    continue;
                }
            }

            if (name != memory::cs2::entity_class::PLAYER_CONTROLLER) continue;

            auto playerOpt = memory::Player::fromController(entity, process, offsets);
            if (!playerOpt.has_value()) continue;

            memory::Player player = playerOpt.value();
            if (!player.isValid(process, offsets)) continue;
            if (player == localPlayer) continue;
            if (spectated_pawn.has_value() && player.getPawn() == spectated_pawn.value()) continue;

            // Create ESP data
            EspPlayerData data;
            data.position = player.position(process, offsets);
            data.head_position = player.bonePosition(process, offsets, static_cast<uint64_t>(memory::Bones::Head));
            data.bones = player.allBones(process, offsets);
            data.name = player.name(process, offsets);
            data.health = player.health(process, offsets);
            data.armor = player.armor(process, offsets);
            data.is_enemy = player.team(process, offsets) != localTeam_;
            data.weapon = player.weapon(process, offsets);
            data.ping = player.ping(process, offsets);
            data.rotation = player.rotation(process, offsets);
            data.pawn = player.getPawn();

            // Check if player is visible (spotted by local player)
            uint64_t pawnIndex = (static_cast<uint64_t>(handle) & 0x7FFF) - 1;
            data.visible = (localSpottedMask & (1LL << pawnIndex)) != 0;

            players_.push_back(std::move(data));
        }
    }

}

bool EspManager::isLocalPlayerHoldingSniper() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Sniper weapons: awp, g3sg1, scar20, ssg08
    if (localWeapon_.empty()) return false;
    
    // Convert to lowercase for comparison
    std::string weapon = localWeapon_;
    for (char& c : weapon) {
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    }
    
    return (weapon == "awp" || 
            weapon == "g3sg1" || 
            weapon == "scar20" || 
            weapon == "ssg08");
}

// World to screen projection - matches Rust implementation exactly
std::optional<ImVec2> WorldToScreen(const memory::Vec3& world_pos,
                                     const memory::Mat4& vm,
                                     const ImVec2& screen_size) {
    // Calculate screen position (x, y)
    float screen_x = vm.x_axis[0] * world_pos.x
                   + vm.x_axis[1] * world_pos.y
                   + vm.x_axis[2] * world_pos.z
                   + vm.x_axis[3];

    float screen_y = vm.y_axis[0] * world_pos.x
                   + vm.y_axis[1] * world_pos.y
                   + vm.y_axis[2] * world_pos.z
                   + vm.y_axis[3];

    // Calculate w for perspective divide
    float w = vm.w_axis[0] * world_pos.x
            + vm.w_axis[1] * world_pos.y
            + vm.w_axis[2] * world_pos.z
            + vm.w_axis[3];

    if (w < 0.0001f) {
        return std::nullopt;  // Behind camera
    }

    // Perspective divide
    screen_x /= w;
    screen_y /= w;

    // Convert to screen coordinates
    float half_w = screen_size.x * 0.5f;
    float half_h = screen_size.y * 0.5f;

    screen_x = half_w + 0.5f * screen_x * screen_size.x + 0.5f;
    screen_y = half_h - 0.5f * screen_y * screen_size.y + 0.5f;

    // Return the position - let ImGui clip if needed
    return ImVec2(screen_x, screen_y);
}

// Render ESP for all players
void RenderEsp(const ImVec2& window_pos, 
               const ImVec2& window_size, 
               const std::vector<EspPlayerData>& players,
               const memory::Mat4& view_matrix) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    auto& settings = g_esp_manager.settings();
    settings.show_box = g_esp_show_box;
    settings.show_skeleton = g_esp_show_skeleton;
    settings.show_health = g_esp_show_health;
    settings.show_name = g_esp_show_name;
    settings.show_weapon = g_esp_show_weapon;
    settings.show_head = g_esp_show_head;
    ImVec2 screen_size = window_size;

    // Render each player
    for (const auto& player : players) {
        // Filter based on team selection from UI
        // g_esp_team_filter: 0 = Enemies only, 1 = Team only, 2 = All
        switch (static_cast<EspTeamFilter>(g_esp_team_filter)) {
            case EspTeamFilter::Enemies:
                if (!player.is_enemy) continue;  // Skip teammates
                break;
            case EspTeamFilter::Team:
                if (player.is_enemy) continue;   // Skip enemies
                break;
            case EspTeamFilter::All:
                // Show everyone
                break;
        }

        // Project feet and head to screen
        auto feetScreen = WorldToScreen(player.position, view_matrix, screen_size);
        auto headScreen = WorldToScreen(player.head_position, view_matrix, screen_size);

        if (!feetScreen.has_value() || !headScreen.has_value()) {
            continue;  // Player not on screen
        }

        ImVec2 feet = feetScreen.value();
        ImVec2 head = headScreen.value();

        // Offset positions by window origin
        feet.x += window_pos.x;
        feet.y += window_pos.y;
        head.x += window_pos.x;
        head.y += window_pos.y;

        // Calculate box dimensions
        float height = feet.y - head.y;
        if (height < 10.0f) continue;  // Too small

        float width = height * 0.4f;
        ImVec2 box_min(head.x - width * 0.5f, head.y);
        ImVec2 box_max(head.x + width * 0.5f, feet.y);

        // Choose color based on team
        ImU32 main_color = player.is_enemy
            ? ImGui::GetColorU32(settings.enemy_color)
            : ImGui::GetColorU32(settings.friendly_color);
        ImU32 text_color = ImGui::GetColorU32(settings.text_color);
        ImU32 bg_color = IM_COL32(0, 0, 0, 100);

        if (g_esp_fake_chams) {
            RenderFakeChams(draw, player, view_matrix, screen_size, window_pos, height, main_color);
        }

        // ESP Box
        if (settings.show_box) {
            draw->AddRect(box_min, box_max, main_color, 0.0f, 0, 2.0f);
        }

        // Health bar
        if (settings.show_health) {
            float hp_pct = static_cast<float>(player.health) / 100.0f;
            hp_pct = ImClamp(hp_pct, 0.0f, 1.0f);

            float bar_width = 4.0f;
            ImVec2 bar_min(box_min.x - 8.0f, box_min.y);
            ImVec2 bar_max(bar_min.x + bar_width, box_max.y);
            float hp_height = (bar_max.y - bar_min.y) * hp_pct;

            // Background
            draw->AddRectFilled(bar_min, bar_max, bg_color, 2.0f);
            draw->AddRect(bar_min, bar_max, text_color, 2.0f);

            // Health fill (green to red gradient)
            ImU32 hp_color = ImGui::GetColorU32(ImLerp(
                ImVec4(0.8f, 0.1f, 0.1f, 1.0f),
                ImVec4(0.1f, 0.8f, 0.1f, 1.0f),
                hp_pct
            ));
            draw->AddRectFilled(ImVec2(bar_min.x, bar_max.y - hp_height), bar_max, hp_color, 2.0f);
        }

        // Skeleton
        if (settings.show_skeleton && !player.bones.empty()) {
            for (const auto& connection : memory::BONE_CONNECTIONS) {
                auto it1 = player.bones.find(connection.first);
                auto it2 = player.bones.find(connection.second);

                if (it1 == player.bones.end() || it2 == player.bones.end()) continue;

                auto screen1 = WorldToScreen(it1->second, view_matrix, screen_size);
                auto screen2 = WorldToScreen(it2->second, view_matrix, screen_size);

                if (!screen1.has_value() || !screen2.has_value()) continue;

                ImVec2 p1(screen1->x + window_pos.x, screen1->y + window_pos.y);
                ImVec2 p2(screen2->x + window_pos.x, screen2->y + window_pos.y);

                draw->AddLine(p1, p2, text_color, 1.5f);
            }
        }

        // Head circle
        if (settings.show_head) {
            float head_radius = height * 0.06f;
            draw->AddCircle(head, head_radius, main_color, 16, 2.0f);
        }

        // Name tag
        if (settings.show_name) {
            const char* name = player.name.empty() ? "Player" : player.name.c_str();
            
            // Append ping if available
            std::string label = name;
            if (player.ping > 0) {
                label += " (" + std::to_string(player.ping) + "ms)";
            }
            
            ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
            ImVec2 text_pos(head.x - text_size.x * 0.5f, box_min.y - text_size.y - 4.0f);

            // Background
            draw->AddRectFilled(
                ImVec2(text_pos.x - 4.0f, text_pos.y - 2.0f),
                ImVec2(text_pos.x + text_size.x + 4.0f, text_pos.y + text_size.y + 2.0f),
                bg_color, 4.0f
            );
            draw->AddText(text_pos, text_color, label.c_str());
        }

        // Weapon name
        if (settings.show_weapon) {
            const char* weapon_name = memory::weaponToString(player.weapon);
            ImVec2 text_size = ImGui::CalcTextSize(weapon_name);
            ImVec2 text_pos(head.x - text_size.x * 0.5f, box_max.y + 4.0f);

            draw->AddRectFilled(
                ImVec2(text_pos.x - 4.0f, text_pos.y - 2.0f),
                ImVec2(text_pos.x + text_size.x + 4.0f, text_pos.y + text_size.y + 2.0f),
                bg_color, 4.0f
            );
            draw->AddText(text_pos, text_color, weapon_name);
        }
    }
}

} // namespace esp
