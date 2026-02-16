#include "state.h"


bool g_see_through_walls = false;
bool g_sniper_crosshair = false;  // Sniper crosshair overlay
bool g_headshoot_line = false;  // Headshoot line overlay
ImVec4 g_headshoot_line_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
bool g_where_is_planted = false;  // Bomb status panel
bool g_aimbot_enabled = false;
int g_aimbot_key_mode = 0;
int g_aimbot_hotkey = 321;  // Mouse5
float g_aimbot_fov = 2.5f;
float g_aimbot_smooth = 5.0f;
int g_aimbot_start_bullet = 0;
bool g_aimbot_distance_adjusted_fov = true;
bool g_aimbot_visibility_check = true;
bool g_aimbot_flash_check = true;
bool g_aimbot_target_friendlies = false;
bool g_aimbot_head_only = false;
bool g_mvp_music_enabled = false;
int g_mvp_music_selection = 0;
int g_esp_team_filter = 0;  // Default: Enemies
int g_esp_mode = 0;  // Default: Basic
bool g_esp_show_box = true;
bool g_esp_show_skeleton = true;
bool g_esp_show_health = true;
bool g_esp_show_name = true;
bool g_esp_show_weapon = true;
bool g_esp_show_head = true;
bool g_esp_fake_chams = false;
std::string g_player_name = "Player";
int g_cs2_ping = 0;
bool g_video_play_request = false;
bool g_video_random_enabled = true;
ImVec2 g_bomb_panel_pos(-1.0f, -1.0f);
ImVec2 g_video_panel_pos(-1.0f, -1.0f);
bool g_watermark_enabled = true;
bool g_grenade_pred = false;
ImVec4 g_grenade_pred_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

// Grenade Proximity Warning
bool g_grenade_proximity_warning = false;
ImVec4 g_grenade_proximity_color = ImVec4(1.0f, 0.3f, 0.0f, 0.35f);
ImVec4 g_grenade_proximity_fire_color = ImVec4(1.0f, 0.15f, 0.0f, 0.25f);

// Radar
bool g_radar_enabled = false;
float g_radar_range = 1050.0f;
float g_radar_size = 205.0f;
ImVec2 g_radar_pos(130.0f, 130.0f);
