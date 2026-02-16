#pragma once

#include <imgui.h>
#include <string>

// ESP team filter options
enum class EspTeamFilter {
    Enemies = 0,
    Team = 1,
    All = 2
};

enum class EspRenderMode {
  Basic = 0,
  FakeChams = 1
};

struct MenuState {
  int page = 0;
  int active_tab = 0;
  float tab_alpha = 0.0f;
  float tab_add = 0.0f;
  float anim_text = 0.0f;
  bool checkbox_active = true;
  bool checkbox_inactive = false;
  float slider_integer = 100.0f;
  float slider_angle = 75.0f;
  float dot_slider = 0.5f;
  char input_text[32] = "Hello mate :\\";
  float accent_color[3] = {0.46f, 0.85f, 0.46f};
  int combo_index = 0;
  int keybind = ImGuiKey_None;
};

extern bool g_see_through_walls;
extern bool g_sniper_crosshair;  // Show crosshair for sniper weapons
extern bool g_headshoot_line;  // Show headshoot line overlay
extern ImVec4 g_headshoot_line_color;
extern bool g_where_is_planted;  // Show bomb status panel
extern bool g_aimbot_enabled;
extern int g_aimbot_key_mode;
extern int g_aimbot_hotkey;
extern float g_aimbot_fov;
extern float g_aimbot_smooth;
extern int g_aimbot_start_bullet;
extern bool g_aimbot_distance_adjusted_fov;
extern bool g_aimbot_visibility_check;
extern bool g_aimbot_flash_check;
extern bool g_aimbot_target_friendlies;
extern bool g_aimbot_head_only;
extern bool g_mvp_music_enabled;
extern int g_mvp_music_selection;
extern int g_esp_team_filter;  // EspTeamFilter cast to int for imgui combo
extern int g_esp_mode;  // EspRenderMode cast to int for imgui combo
extern bool g_esp_show_box;
extern bool g_esp_show_skeleton;
extern bool g_esp_show_health;
extern bool g_esp_show_name;
extern bool g_esp_show_weapon;
extern bool g_esp_show_head;
extern bool g_esp_fake_chams;
extern std::string g_player_name;
extern int g_cs2_ping;
extern bool g_video_play_request;
extern bool g_video_random_enabled;
extern ImVec2 g_bomb_panel_pos;
extern ImVec2 g_video_panel_pos;
extern bool g_watermark_enabled;
extern bool g_grenade_pred;
extern ImVec4 g_grenade_pred_color;

// Grenade Proximity Warning
extern bool g_grenade_proximity_warning;
extern ImVec4 g_grenade_proximity_color;
extern ImVec4 g_grenade_proximity_fire_color;

// Radar
extern bool g_radar_enabled;
extern float g_radar_range;
extern float g_radar_size;
extern ImVec2 g_radar_pos;
