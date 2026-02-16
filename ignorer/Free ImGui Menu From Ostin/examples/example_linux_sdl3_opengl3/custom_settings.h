#pragma once

#include "imgui.h"
#include <string_view>
#include <vector>


class c_settings
{
public:
    ImVec2 window_size = ImVec2(800, 600);
    ImVec2 particle_size = ImVec2(1920, 1080);
    float window_sidebar_width = 200.f;
    ImVec2 window_padding = ImVec2(0, 0);
    ImVec2 window_item_spacing = ImVec2(0, 0);
    float window_border_size = 0.f;
    float window_shadow_size = 0.f;
    float window_rounding = 8.f;
    float window_scrollbar_size = 7.f;
    float window_scrollbar_rounding = 5.f;
    std::string_view window_name = "ExactlyXiters X";

    int selection_count = 0;
    const char* selection_names[4] = { "AimHacks", "Misc", "Visuals", "Settings" };

    int aim_sub_selection_count = 0;
    const char* aim_sub_selection_names[3] = { "Ragebot", "Legitbot", "Anti-Aims" };
    int visuals_sub_selection_count = 0;
    const char* visuals_sub_selection_names[3] = { "Self", "Team", "Enemy" };
    int config_sub_selection_count = 0;
    const char* config_sub_selection_names[2] = { "Local", "Cloud" };

    float config_child_height = 0.f;
    int config_active = 0;

    float widgets_rounding = 2.f;
    ImVec2 widgets_spacing = ImVec2(17, 17);
    ImVec2 widgets_padding = ImVec2(10, 10);
    ImVec2 combo_spacing = ImVec2(10, 10);

    ImVec4 colorpicker_temp_buf = ImVec4(-1.f, -1.f, -1.f, -1.f);
    std::vector<ImVec4> colorpicker_stored_colors;

    ImFont* window_name_font = nullptr;
    ImFont* selection_font = nullptr;
    ImFont* child_font = nullptr;
    ImFont* widgets_font = nullptr;
    ImFont* value_font = nullptr;
    ImFont* dropdown_expand_font = nullptr;
    ImFont* selectable_icon_font = nullptr;
    ImFont* colorpicker_icon_font = nullptr;
    ImFont* key_icon_font = nullptr;
    ImFont* config_font = nullptr;
    ImFont* config_icon_font = nullptr;
    ImFont* settings_font = nullptr;
    ImFont* watermark_font = nullptr;
    ImFont* watermark_small_font = nullptr;
    ImFont* watermark_icon_font = nullptr;


    unsigned int logotype_texture_dx11 = 0;
};

inline c_settings* set = new c_settings();
