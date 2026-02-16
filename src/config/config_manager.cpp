#include "config_manager.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "functionalities/clickity/clickity.h"
#include "ui/state.h"

namespace {

bool ParseBool(const std::string &value, bool &out) {
  if (value == "1" || value == "true" || value == "TRUE" || value == "True") {
    out = true;
    return true;
  }
  if (value == "0" || value == "false" || value == "FALSE" || value == "False") {
    out = false;
    return true;
  }
  return false;
}

}  // namespace

namespace config {

bool SaveConfig(const std::string &path) {
  std::filesystem::path out_path(path);
  if (out_path.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(out_path.parent_path(), ec);
  }

  std::ofstream out(path, std::ios::out | std::ios::trunc);
  if (!out.is_open()) {
    return false;
  }

  auto &clickity = functionalities::Clickity::Get();

  out << "see_through_walls=" << (g_see_through_walls ? 1 : 0) << "\n";
  out << "sniper_crosshair=" << (g_sniper_crosshair ? 1 : 0) << "\n";
  out << "headshoot_line=" << (g_headshoot_line ? 1 : 0) << "\n";
  out << "headshoot_line_color_r=" << g_headshoot_line_color.x << "\n";
  out << "headshoot_line_color_g=" << g_headshoot_line_color.y << "\n";
  out << "headshoot_line_color_b=" << g_headshoot_line_color.z << "\n";
  out << "headshoot_line_color_a=" << g_headshoot_line_color.w << "\n";
  out << "where_is_planted=" << (g_where_is_planted ? 1 : 0) << "\n";
  out << "aimbot_enabled=" << (g_aimbot_enabled ? 1 : 0) << "\n";
  out << "aimbot_key_mode=" << g_aimbot_key_mode << "\n";
  out << "aimbot_hotkey=" << g_aimbot_hotkey << "\n";
  out << "aimbot_fov=" << g_aimbot_fov << "\n";
  out << "aimbot_smooth=" << g_aimbot_smooth << "\n";
  out << "aimbot_start_bullet=" << g_aimbot_start_bullet << "\n";
  out << "aimbot_distance_adjusted_fov=" << (g_aimbot_distance_adjusted_fov ? 1 : 0) << "\n";
  out << "aimbot_visibility_check=" << (g_aimbot_visibility_check ? 1 : 0) << "\n";
  out << "aimbot_flash_check=" << (g_aimbot_flash_check ? 1 : 0) << "\n";
  out << "aimbot_target_friendlies=" << (g_aimbot_target_friendlies ? 1 : 0) << "\n";
  out << "aimbot_head_only=" << (g_aimbot_head_only ? 1 : 0) << "\n";
  out << "esp_team_filter=" << g_esp_team_filter << "\n";
  out << "esp_mode=" << g_esp_mode << "\n";
  out << "esp_show_box=" << (g_esp_show_box ? 1 : 0) << "\n";
  out << "esp_show_skeleton=" << (g_esp_show_skeleton ? 1 : 0) << "\n";
  out << "esp_show_health=" << (g_esp_show_health ? 1 : 0) << "\n";
  out << "esp_show_name=" << (g_esp_show_name ? 1 : 0) << "\n";
  out << "esp_show_weapon=" << (g_esp_show_weapon ? 1 : 0) << "\n";
  out << "esp_show_head=" << (g_esp_show_head ? 1 : 0) << "\n";
  out << "esp_fake_chams=" << (g_esp_fake_chams ? 1 : 0) << "\n";
  out << "mvp_music_enabled=" << (g_mvp_music_enabled ? 1 : 0) << "\n";
  out << "mvp_music_selection=" << g_mvp_music_selection << "\n";
  out << "video_random_enabled=" << (g_video_random_enabled ? 1 : 0) << "\n";
  out << "bomb_panel_pos_x=" << g_bomb_panel_pos.x << "\n";
  out << "bomb_panel_pos_y=" << g_bomb_panel_pos.y << "\n";
  out << "video_panel_pos_x=" << g_video_panel_pos.x << "\n";
  out << "video_panel_pos_y=" << g_video_panel_pos.y << "\n";
  out << "watermark_enabled=" << (g_watermark_enabled ? 1 : 0) << "\n";
  out << "grenade_pred=" << (g_grenade_pred ? 1 : 0) << "\n";
  out << "grenade_pred_color_r=" << g_grenade_pred_color.x << "\n";
  out << "grenade_pred_color_g=" << g_grenade_pred_color.y << "\n";
  out << "grenade_pred_color_b=" << g_grenade_pred_color.z << "\n";
  out << "grenade_pred_color_a=" << g_grenade_pred_color.w << "\n";

  out << "clickity_enabled=" << (clickity.enabled_ ? 1 : 0) << "\n";
  out << "clickity_team_check=" << (clickity.team_check_ ? 1 : 0) << "\n";
  out << "clickity_flash_check=" << (clickity.flash_check_ ? 1 : 0) << "\n";
  out << "clickity_scope_check=" << (clickity.scope_check_ ? 1 : 0) << "\n";
  out << "clickity_velocity_check=" << (clickity.velocity_check_ ? 1 : 0) << "\n";
  out << "clickity_delay_start=" << clickity.delay_start_ << "\n";
  out << "clickity_delay_end=" << clickity.delay_end_ << "\n";
  out << "clickity_shot_duration=" << clickity.shot_duration_ << "\n";
  out << "clickity_click_delay=" << clickity.click_delay_ << "\n";

  return true;
}

bool LoadConfig(const std::string &path) {
  std::ifstream in(path);
  if (!in.is_open()) {
    return false;
  }

  auto &clickity = functionalities::Clickity::Get();
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    auto eq_pos = line.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, eq_pos);
    std::string value = line.substr(eq_pos + 1);

    if (key == "see_through_walls") {
      ParseBool(value, g_see_through_walls);
    } else if (key == "sniper_crosshair") {
      ParseBool(value, g_sniper_crosshair);
    } else if (key == "headshoot_line") {
      ParseBool(value, g_headshoot_line);
    } else if (key == "headshoot_line_color_r") {
      g_headshoot_line_color.x = std::stof(value);
    } else if (key == "headshoot_line_color_g") {
      g_headshoot_line_color.y = std::stof(value);
    } else if (key == "headshoot_line_color_b") {
      g_headshoot_line_color.z = std::stof(value);
    } else if (key == "headshoot_line_color_a") {
      g_headshoot_line_color.w = std::stof(value);
    } else if (key == "where_is_planted") {
      ParseBool(value, g_where_is_planted);
    } else if (key == "aimbot_enabled") {
      ParseBool(value, g_aimbot_enabled);
    } else if (key == "aimbot_key_mode") {
      g_aimbot_key_mode = std::stoi(value);
    } else if (key == "aimbot_hotkey") {
      g_aimbot_hotkey = std::stoi(value);
    } else if (key == "aimbot_fov") {
      g_aimbot_fov = std::stof(value);
    } else if (key == "aimbot_smooth") {
      g_aimbot_smooth = std::stof(value);
    } else if (key == "aimbot_start_bullet") {
      g_aimbot_start_bullet = std::stoi(value);
    } else if (key == "aimbot_distance_adjusted_fov") {
      ParseBool(value, g_aimbot_distance_adjusted_fov);
    } else if (key == "aimbot_visibility_check") {
      ParseBool(value, g_aimbot_visibility_check);
    } else if (key == "aimbot_flash_check") {
      ParseBool(value, g_aimbot_flash_check);
    } else if (key == "aimbot_target_friendlies") {
      ParseBool(value, g_aimbot_target_friendlies);
    } else if (key == "aimbot_head_only") {
      ParseBool(value, g_aimbot_head_only);
    } else if (key == "esp_team_filter") {
      g_esp_team_filter = std::stoi(value);
    } else if (key == "esp_mode") {
      g_esp_mode = std::stoi(value);
    } else if (key == "esp_show_box") {
      ParseBool(value, g_esp_show_box);
    } else if (key == "esp_show_skeleton") {
      ParseBool(value, g_esp_show_skeleton);
    } else if (key == "esp_show_health") {
      ParseBool(value, g_esp_show_health);
    } else if (key == "esp_show_name") {
      ParseBool(value, g_esp_show_name);
    } else if (key == "esp_show_weapon") {
      ParseBool(value, g_esp_show_weapon);
    } else if (key == "esp_show_head") {
      ParseBool(value, g_esp_show_head);
    } else if (key == "esp_fake_chams") {
      ParseBool(value, g_esp_fake_chams);
    } else if (key == "mvp_music_enabled") {
      ParseBool(value, g_mvp_music_enabled);
    } else if (key == "mvp_music_selection") {
      g_mvp_music_selection = std::stoi(value);
    } else if (key == "video_random_enabled") {
      ParseBool(value, g_video_random_enabled);
    } else if (key == "bomb_panel_pos_x") {
      g_bomb_panel_pos.x = std::stof(value);
    } else if (key == "bomb_panel_pos_y") {
      g_bomb_panel_pos.y = std::stof(value);
    } else if (key == "video_panel_pos_x") {
      g_video_panel_pos.x = std::stof(value);
    } else if (key == "video_panel_pos_y") {
      g_video_panel_pos.y = std::stof(value);
    } else if (key == "watermark_enabled") {
      ParseBool(value, g_watermark_enabled);
    } else if (key == "grenade_pred") {
      ParseBool(value, g_grenade_pred);
    } else if (key == "grenade_pred_color_r") {
      g_grenade_pred_color.x = std::stof(value);
    } else if (key == "grenade_pred_color_g") {
      g_grenade_pred_color.y = std::stof(value);
    } else if (key == "grenade_pred_color_b") {
      g_grenade_pred_color.z = std::stof(value);
    } else if (key == "grenade_pred_color_a") {
      g_grenade_pred_color.w = std::stof(value);
    } else if (key == "clickity_enabled") {
      ParseBool(value, clickity.enabled_);
    } else if (key == "clickity_team_check") {
      ParseBool(value, clickity.team_check_);
    } else if (key == "clickity_flash_check") {
      ParseBool(value, clickity.flash_check_);
    } else if (key == "clickity_scope_check") {
      ParseBool(value, clickity.scope_check_);
    } else if (key == "clickity_velocity_check") {
      ParseBool(value, clickity.velocity_check_);
    } else if (key == "clickity_delay_start") {
      clickity.delay_start_ = std::stoi(value);
    } else if (key == "clickity_delay_end") {
      clickity.delay_end_ = std::stoi(value);
    } else if (key == "clickity_shot_duration") {
      clickity.shot_duration_ = std::stoi(value);
    } else if (key == "clickity_click_delay") {
      clickity.click_delay_ = std::stoi(value);
    }
  }

  return true;
}

}  // namespace config
