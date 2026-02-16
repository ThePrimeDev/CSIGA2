#include "radar.h"

#include <cmath>
#include <cstring>
#include <string>

#include "../../functionalities/visuals/see-trough/esp.h"
#include "../../memory/memory.h"
#include "../../ui/state.h"

#include <imgui.h>

namespace functionalities {

namespace {

constexpr float kPi = 3.14159265358979323846f;

// CS2 radar HUD constants (at 1920x1080, default HUD scale)
// The in-game radar is a square panel in the top-left corner
// Centered at roughly (128, 128) with ~120px effective radius
constexpr float kDefaultRadarCenterX = 130.0f;
constexpr float kDefaultRadarCenterY = 130.0f;

struct RadarPlayer {
    float screen_x;
    float screen_y;
    float yaw;       // player's facing direction
    bool is_enemy;
    int health;
};

// Draw a directional arrow (diamond/kite shape) like the CS2 radar dots
void DrawPlayerArrow(ImDrawList* draw, ImVec2 pos, float yaw_deg, float local_yaw_deg,
                     float arrow_size, ImU32 fill_color, ImU32 outline_color) {
    // Angle relative to local player's view (pointing direction of the target)
    float angle_rad = (local_yaw_deg - yaw_deg + 180.0f) * (kPi / 180.0f);

    float cos_a = std::cos(angle_rad);
    float sin_a = std::sin(angle_rad);

    // Arrow shape: forward point, two back corners, and center-back
    float front = arrow_size;
    float back = arrow_size * 0.5f;
    float side = arrow_size * 0.6f;

    // Tip (forward)
    ImVec2 tip(pos.x + sin_a * front, pos.y - cos_a * front);
    // Left wing
    ImVec2 left(pos.x + std::sin(angle_rad + 2.3f) * side,
                pos.y - std::cos(angle_rad + 2.3f) * side);
    // Right wing
    ImVec2 right(pos.x + std::sin(angle_rad - 2.3f) * side,
                 pos.y - std::cos(angle_rad - 2.3f) * side);
    // Tail
    ImVec2 tail(pos.x - sin_a * back * 0.3f, pos.y + cos_a * back * 0.3f);

    // Draw filled diamond/kite
    draw->AddQuadFilled(tip, left, tail, right, fill_color);
    draw->AddQuad(tip, left, tail, right, outline_color, 1.0f);
}

} // namespace

void DrawRadar(bool menu_visible) {
    if (!g_radar_enabled) return;

    if (!esp::g_esp_manager.isConnected()) return;

    const auto& process_opt = esp::g_esp_manager.getProcess();
    const auto& offsets_opt = esp::g_esp_manager.getOffsets();
    if (!process_opt.has_value() || !offsets_opt.has_value()) return;

    const auto& process = process_opt.value();
    const auto& offsets = offsets_opt.value();

    auto local_player_opt = memory::Player::localPlayer(process, offsets);
    if (!local_player_opt.has_value()) return;

    const auto& local_player = local_player_opt.value();
    if (!local_player.isValid(process, offsets)) return;

    memory::Vec3 local_pos = local_player.position(process, offsets);
    memory::Vec2 view_angles = local_player.viewAngles(process, offsets);
    uint8_t local_team = local_player.team(process, offsets);
    float local_yaw = view_angles.y;

    float radar_half = g_radar_size * 0.5f;
    // Use saved position; default aligns with CS2 radar
    if (g_radar_pos.x < 0) g_radar_pos.x = kDefaultRadarCenterX;
    if (g_radar_pos.y < 0) g_radar_pos.y = kDefaultRadarCenterY;
    ImVec2 center = g_radar_pos;

    // Arrow size scales with radar
    float arrow_size = g_radar_size * 0.03f;
    if (arrow_size < 4.0f) arrow_size = 4.0f;

    // Proportion: how many game units fit in the radar half-width
    float proportion = g_radar_range;

    // Collect players
    std::vector<RadarPlayer> players;

    // Use cached players from EspManager instead of re-scanning entities
    for (const auto& player : esp::g_esp_manager.getPlayers()) {
        memory::Vec3 player_pos = player.position;
        uint8_t player_team = player.is_enemy ? (local_team == 2 ? 3 : 2) : local_team; // Infer team from is_enemy if needed, but we have is_enemy flag directly
        // Actually we just need to know if enemy or not for color
        bool is_enemy = player.is_enemy;
        int health = player.health;
        float player_yaw = player.rotation;

        // Calculate radar position using atan2 rotation (matching CS2_External method)
        float dx = player_pos.x - local_pos.x;
        float dy = player_pos.y - local_pos.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        float angle = std::atan2(dy, dx) * 180.0f / kPi;

        // Rotate by local yaw
        float rotated_angle = (local_yaw - angle) * kPi / 180.0f;

        // Scale distance to radar coordinates
        float scaled_dist = distance / proportion * radar_half;

        // Compute screen position
        float sx = center.x + scaled_dist * std::sin(rotated_angle);
        float sy = center.y - scaled_dist * std::cos(rotated_angle);

        // Clip to radar square (like CS2 radar)
        if (sx < center.x - radar_half || sx > center.x + radar_half ||
            sy < center.y - radar_half || sy > center.y + radar_half) {
            // Clamp to edge
            sx = std::max(center.x - radar_half + arrow_size, std::min(sx, center.x + radar_half - arrow_size));
            sy = std::max(center.y - radar_half + arrow_size, std::min(sy, center.y + radar_half - arrow_size));
        }

        RadarPlayer rp;
        rp.screen_x = sx;
        rp.screen_y = sy;
        rp.yaw = player_yaw;
        rp.is_enemy = is_enemy;
        rp.health = health;
        players.push_back(rp);
    }

    // When menu is open: draggable ImGui window
    // When menu is closed: draw to background (non-interactive)
    if (menu_visible) {
        ImVec2 win_pos(center.x - radar_half - 5.0f, center.y - radar_half - 5.0f);
        ImVec2 win_size(g_radar_size + 10.0f, g_radar_size + 10.0f);
        ImGui::SetNextWindowPos(win_pos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(win_size);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoBackground;

        if (ImGui::Begin("##RadarDrag", nullptr, flags)) {
            // Save the dragged position (center = window_pos + half_size + padding)
            ImVec2 wp = ImGui::GetWindowPos();
            g_radar_pos = ImVec2(wp.x + g_radar_size * 0.5f + 5.0f,
                                 wp.y + g_radar_size * 0.5f + 5.0f);
            center = g_radar_pos;

            // Draw a visible border so user knows it's draggable
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 tl(center.x - radar_half, center.y - radar_half);
            ImVec2 br(center.x + radar_half, center.y + radar_half);
            draw->AddRectFilled(tl, br, IM_COL32(10, 10, 10, 140), 4.0f);
            draw->AddRect(tl, br, IM_COL32(100, 200, 100, 200), 4.0f, 0, 2.0f);  // Green border = draggable

            draw->AddLine(ImVec2(center.x, tl.y), ImVec2(center.x, br.y), IM_COL32(50, 50, 50, 100), 1.0f);
            draw->AddLine(ImVec2(tl.x, center.y), ImVec2(br.x, center.y), IM_COL32(50, 50, 50, 100), 1.0f);
            draw->AddCircleFilled(center, 3.0f, IM_COL32(255, 255, 255, 220), 12);

            for (const auto& rp : players) {
                ImVec2 pos(rp.screen_x, rp.screen_y);
                ImU32 fill_color;
                if (rp.is_enemy) {
                    float alpha = 0.6f + 0.4f * (rp.health / 100.0f);
                    fill_color = IM_COL32(235, 64, 52, static_cast<int>(255 * alpha));
                } else {
                    fill_color = IM_COL32(75, 140, 235, 200);
                }
                DrawPlayerArrow(draw, pos, rp.yaw, local_yaw, arrow_size, fill_color, IM_COL32(0, 0, 0, 150));
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    } else {
        // Non-interactive: draw to background draw list
        ImDrawList* draw = ImGui::GetBackgroundDrawList();

        ImVec2 tl(center.x - radar_half, center.y - radar_half);
        ImVec2 br(center.x + radar_half, center.y + radar_half);
        draw->AddRectFilled(tl, br, IM_COL32(10, 10, 10, 120), 4.0f);
        draw->AddRect(tl, br, IM_COL32(40, 40, 40, 150), 4.0f, 0, 1.0f);

        draw->AddLine(ImVec2(center.x, tl.y), ImVec2(center.x, br.y), IM_COL32(50, 50, 50, 100), 1.0f);
        draw->AddLine(ImVec2(tl.x, center.y), ImVec2(br.x, center.y), IM_COL32(50, 50, 50, 100), 1.0f);
        draw->AddCircleFilled(center, 3.0f, IM_COL32(255, 255, 255, 220), 12);

        for (const auto& rp : players) {
            ImVec2 pos(rp.screen_x, rp.screen_y);
            ImU32 fill_color;
            if (rp.is_enemy) {
                float alpha = 0.6f + 0.4f * (rp.health / 100.0f);
                fill_color = IM_COL32(235, 64, 52, static_cast<int>(255 * alpha));
            } else {
                fill_color = IM_COL32(75, 140, 235, 200);
            }
            DrawPlayerArrow(draw, pos, rp.yaw, local_yaw, arrow_size, fill_color, IM_COL32(0, 0, 0, 150));
        }
    }
}

} // namespace functionalities
