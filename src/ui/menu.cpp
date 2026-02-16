#include "menu.h"

#include "assets.h"
#include "ostin_style.h"
#include "widgets.h"
#include "../functionalities/aimbot/aimbot.h"
#include "../functionalities/clickity/clickity.h"
#include "config/config_manager.h"
#ifdef CSIGA2_FUN
#include "audio/virtual_mic.h"
#endif

#include <imgui.h>
#include <imgui_internal.h>

#include <cfloat>
#include <cstdio>

namespace {

ImFont *PickFont(ImFont *font, ImFont *fallback) {
  return font ? font : fallback;
}

void DrawShadowCircle(ImDrawList *draw_list,
                      ImVec2 center,
                      float radius,
                      ImU32 color,
                      float spread) {
  const int steps = 16;
  ImVec4 base = ImGui::ColorConvertU32ToFloat4(color);
  for (int i = 0; i < steps; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(steps);
    float alpha = (1.0f - t) * 0.35f;
    ImVec4 c = base;
    c.w *= alpha;
    draw_list->AddCircleFilled(center, radius + spread * t, ImGui::GetColorU32(c), 64);
  }
}

bool TabButton(const char *label,
               const char *icon,
               bool active,
               ImVec2 size,
               ImFont *text_font,
               ImFont *icon_font) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  ImGuiID id = window->GetID(label);
  ImVec2 pos = window->DC.CursorPos;
  ImRect rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ImGui::ItemSize(rect);
  if (!ImGui::ItemAdd(rect, id)) {
    return false;
  }

  bool hovered = false;
  bool held = false;
  bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

  ImU32 bg = ImGui::GetColorU32(active ? ImVec4(0.07f, 0.07f, 0.08f, 1.0f)
                                      : ImVec4(0.04f, 0.04f, 0.05f, 1.0f));
  ImU32 border = ImGui::GetColorU32(ImVec4(0.13f, 0.13f, 0.15f, 1.0f));
  window->DrawList->AddRectFilled(rect.Min, rect.Max, bg, 8.0f);
  window->DrawList->AddRect(rect.Min, rect.Max, border, 8.0f);

  ImVec2 text_pos = ImVec2(rect.Min.x + 54.0f, rect.Min.y + 16.0f);
  ImVec2 icon_pos = ImVec2(rect.Min.x + 18.0f, rect.Min.y + 10.0f);

  if (icon && icon[0] != '\0') {
    ImGui::PushFont(icon_font);
    const float icon_size = ImGui::GetFontSize();
    window->DrawList->AddText(icon_font, icon_size, icon_pos,
                              ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), icon);
    ImGui::PopFont();
  }

  ImGui::PushFont(text_font);
  const float text_size = ImGui::GetFontSize();
  window->DrawList->AddText(text_font, text_size, text_pos,
                            ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), label);
  ImGui::PopFont();

  return pressed;
}

}  // namespace

void RenderWatermark() {
  UiAssets &assets = GetUiAssets();
  csiga2_style::Colors &colors = csiga2_style::GetColors();

  ImFont *base_font = PickFont(assets.inter_s, ImGui::GetFont());
  ImFont *small_font = PickFont(assets.inter_s3, base_font);
  auto TextWidth = [](ImFont *font, float size, const char *text) {
    return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
  };

  const float icon_size = 20.0f;
  const float icon_text_gap = 8.0f;
  const float block_gap = 18.0f;
  const float pad = 16.0f;

  // Pre-calculate total content width
  float total_w = pad;  // left padding
  total_w += icon_size + icon_text_gap;  // logo icon
  total_w += TextWidth(base_font, 17.0f, "CSIGA2") + 6.0f;
  total_w += TextWidth(base_font, 17.0f, "MENU") + block_gap;
  total_w += icon_size + icon_text_gap;  // user icon
  total_w += TextWidth(base_font, 17.0f, g_player_name.c_str()) + block_gap;
  total_w += icon_size + icon_text_gap;  // fps icon
  total_w += TextWidth(base_font, 17.0f, "999") + 6.0f;  // fps digits
  total_w += TextWidth(small_font, 15.0f, "FPS") + block_gap;
  total_w += icon_size + icon_text_gap;  // ms icon
  total_w += TextWidth(base_font, 17.0f, "999") + 6.0f;  // ping digits
  total_w += TextWidth(small_font, 15.0f, "MS");
  total_w += pad;  // right padding

  ImGui::SetNextWindowSize(ImVec2(total_w, 50));
  ImGui::SetNextWindowPos(ImVec2(10, 10));
  ImGui::Begin("##watermark", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);
  {
    const ImVec2 p = ImGui::GetWindowPos();
    ImDrawList *draw = ImGui::GetWindowDrawList();

    const float y_icon = p.y + 15.0f;
    const float y_text = p.y + 16.0f;
    const float y_small = p.y + 18.0f;

    float x = p.x + pad;
    if (assets.icon_logo) {
      draw->AddImage(assets.icon_logo, ImVec2(x, y_icon),
                     ImVec2(x + icon_size, y_icon + icon_size),
                     ImVec2(0, 0), ImVec2(1, 1));
    } else {
      draw->AddCircleFilled(ImVec2(x + icon_size * 0.5f, y_icon + icon_size * 0.5f),
                            icon_size * 0.5f,
                            ImGui::GetColorU32(colors.main_color), 32);
    }
    x += icon_size + icon_text_gap;
    draw->AddText(base_font, 17.0f, ImVec2(x, y_text),
                  ImGui::GetColorU32(colors.main_color), "CSIGA2");
    x += TextWidth(base_font, 17.0f, "CSIGA2") + 6.0f;
    draw->AddText(base_font, 17.0f, ImVec2(x, y_text),
                  ImGui::GetColorU32(colors.text_active), "MENU");
    x += TextWidth(base_font, 17.0f, "MENU") + block_gap;

    if (assets.icon_user) {
      draw->AddImage(assets.icon_user, ImVec2(x, y_icon),
                     ImVec2(x + icon_size - 2, y_icon + icon_size - 2),
                     ImVec2(0, 0), ImVec2(1, 1),
                     ImGui::GetColorU32(colors.main_color));
    } else {
      draw->AddCircleFilled(ImVec2(x + icon_size * 0.5f, y_icon + icon_size * 0.5f),
                            icon_size * 0.5f,
                            ImGui::GetColorU32(colors.main_color), 32);
    }
    x += icon_size + icon_text_gap;
    draw->AddText(base_font, 17.0f, ImVec2(x, y_text),
                  ImGui::GetColorU32(colors.text_active), g_player_name.c_str());
    x += TextWidth(base_font, 17.0f, g_player_name.c_str()) + block_gap;

    if (assets.icon_fps) {
      draw->AddImage(assets.icon_fps, ImVec2(x, y_icon),
                     ImVec2(x + icon_size, y_icon + icon_size),
                     ImVec2(0, 0), ImVec2(1, 1),
                     ImGui::GetColorU32(colors.main_color));
    } else {
      draw->AddCircleFilled(ImVec2(x + icon_size * 0.5f, y_icon + icon_size * 0.5f),
                            icon_size * 0.5f,
                            ImGui::GetColorU32(colors.main_color), 32);
    }
    x += icon_size + icon_text_gap;
    // Calculate actual FPS from ImGui framerate
    char fps_buf[16];
    int fps = static_cast<int>(ImGui::GetIO().Framerate);
    snprintf(fps_buf, sizeof(fps_buf), "%d", fps);
    draw->AddText(base_font, 17.0f, ImVec2(x, y_text),
                  ImGui::GetColorU32(colors.text_active), fps_buf);
    x += TextWidth(base_font, 17.0f, fps_buf) + 6.0f;
    draw->AddText(small_font, 15.0f, ImVec2(x, y_small),
                  ImGui::GetColorU32(colors.text_inactive), "FPS");
    x += TextWidth(small_font, 15.0f, "FPS") + block_gap;

    if (assets.icon_ms) {
      draw->AddImage(assets.icon_ms, ImVec2(x, y_icon),
                     ImVec2(x + icon_size, y_icon + icon_size),
                     ImVec2(0, 0), ImVec2(1, 1),
                     ImGui::GetColorU32(colors.main_color));
    } else {
      draw->AddCircleFilled(ImVec2(x + icon_size * 0.5f, y_icon + icon_size * 0.5f),
                            icon_size * 0.5f,
                            ImGui::GetColorU32(colors.main_color), 32);
    }
    x += icon_size + icon_text_gap;
    // Display ping from CS2 (if connected)
    char ping_buf[16];
    snprintf(ping_buf, sizeof(ping_buf), "%d", g_cs2_ping);
    draw->AddText(base_font, 17.0f, ImVec2(x, y_text),
                  ImGui::GetColorU32(colors.text_active), ping_buf);
    x += TextWidth(base_font, 17.0f, ping_buf) + 6.0f;
    draw->AddText(small_font, 15.0f, ImVec2(x, y_small),
                  ImGui::GetColorU32(colors.text_inactive), "MS");
  }
  ImGui::End();
}

void RenderStatusIndicator() {
  if (!g_aimbot_enabled) {
    return;
  }

  UiAssets &assets = GetUiAssets();
  csiga2_style::Colors &colors = csiga2_style::GetColors();
  ImFont *base_font = PickFont(assets.inter_s, ImGui::GetFont());

  auto TextWidth = [](ImFont *font, float size, const char *text) {
    return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
  };

  bool is_active = (g_aimbot_key_mode == 0)
      ? false  // hold mode: no persistent indicator needed
      : functionalities::Aimbot::Get().isActive();
  const char *mode_label = (g_aimbot_key_mode == 0) ? "HOLD" : "TOGGLE";
  const char *state_label = (g_aimbot_key_mode == 0)
      ? "AIMBOT"
      : (is_active ? "AIMBOT ON" : "AIMBOT OFF");

  const float pad = 12.0f;
  const float dot_r = 5.0f;
  const float dot_gap = 8.0f;
  float text_w = TextWidth(base_font, 15.0f, state_label);
  float mode_w = TextWidth(base_font, 13.0f, mode_label);
  float total_w = pad + dot_r * 2.0f + dot_gap + text_w + 12.0f + mode_w + pad;

  ImGui::SetNextWindowSize(ImVec2(total_w, 32));
  ImGui::SetNextWindowPos(ImVec2(10, 66));
  ImGui::Begin("##status_indicator", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                   ImGuiWindowFlags_NoInputs);
  {
    const ImVec2 p = ImGui::GetWindowPos();
    ImDrawList *draw = ImGui::GetWindowDrawList();

    float x = p.x + pad;
    float cy = p.y + 16.0f;

    // Status dot
    ImU32 dot_color;
    if (g_aimbot_key_mode == 0) {
      dot_color = ImGui::GetColorU32(colors.main_color);
    } else if (is_active) {
      dot_color = IM_COL32(80, 220, 80, 255);  // green
    } else {
      dot_color = IM_COL32(120, 120, 120, 200);  // dimmed gray
    }
    draw->AddCircleFilled(ImVec2(x + dot_r, cy), dot_r, dot_color, 16);
    x += dot_r * 2.0f + dot_gap;

    // State label
    ImU32 text_color = is_active || g_aimbot_key_mode == 0
        ? ImGui::GetColorU32(colors.text_active)
        : ImGui::GetColorU32(colors.text_inactive);
    draw->AddText(base_font, 15.0f, ImVec2(x, p.y + 9.0f), text_color, state_label);
    x += text_w + 12.0f;

    // Mode badge
    draw->AddText(base_font, 13.0f, ImVec2(x, p.y + 10.0f),
                  ImGui::GetColorU32(colors.text_inactive), mode_label);
  }
  ImGui::End();
}

void RenderMenu(MenuState &state, bool *ui_visible, bool *app_running) {
  UiAssets &assets = GetUiAssets();
  csiga2_style::Colors &colors = csiga2_style::GetColors();
  ImGuiIO &io = ImGui::GetIO();

  ImGui::SetNextWindowSize(ImVec2(1050, 650));
  ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 1050) * 0.5f,
                                 (io.DisplaySize.y - 650) * 0.5f),
                          ImGuiCond_Always);
  ImGui::Begin("General", ui_visible,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration |
             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
             ImGuiWindowFlags_NoBringToFrontOnFocus |
             ImGuiWindowFlags_NoBackground);
  {
    ImDrawList *draw = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 region = ImGui::GetContentRegionMax();

    if (assets.background) {
      ImVec2 window_size = ImGui::GetWindowSize();
      ImVec2 max = ImVec2(p.x + window_size.x, p.y + window_size.y);
      draw->PushClipRect(p, max, true);
      draw->AddImage(assets.background, p, max);
      draw->PopClipRect();
    }

    state.tab_alpha = ImLerp(state.tab_alpha, (state.page == state.active_tab) ? 1.0f : 0.0f,
                             18.0f * io.DeltaTime);
    if (state.tab_alpha < 0.01f && state.tab_add < 0.01f) {
      state.active_tab = state.page;
    }


    const float left_inset = 27.0f;
    const float panel_gap = 32.0f;
    const float title_size = 34.0f;
    const float button_height = 50.0f;
    const float menu_width = 234.0f;
    const float content_gap = 48.0f;
    const float content_left = left_inset + menu_width + content_gap;
    const float content_right_inset = 24.0f;

    ImFont *title_font = PickFont(assets.inter_b, ImGui::GetFont());
    const float title_y = p.y + panel_gap;
    draw->AddText(title_font, title_size, ImVec2(p.x + left_inset, title_y),
            ImGui::GetColorU32(colors.main_color), "Counter-Strike Intelligent");
    draw->AddText(title_font, title_size, ImVec2(p.x + left_inset, title_y + title_size + 4.0f),
            ImGui::GetColorU32(colors.text_active), "Game Assistant 2");

    ImGui::SetCursorPos(ImVec2(left_inset, panel_gap + title_size + panel_gap));
    ImGui::BeginGroup();
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));
      ImFont *text_font = PickFont(assets.inter_s, ImGui::GetFont());
      ImFont *icon_font = PickFont(assets.icon, ImGui::GetFont());

      if (TabButton("AimBot", "A", 0 == state.page, ImVec2(234, 50), text_font,
                    icon_font)) {
        state.page = 0;
      }
      if (TabButton("Clickity", "L", 1 == state.page, ImVec2(234, 50), text_font,
                    icon_font)) {
        state.page = 1;
      }
      if (TabButton("Visuals", "V", 2 == state.page, ImVec2(234, 50), text_font,
                    icon_font)) {
        state.page = 2;
      }
      if (TabButton("Settings", "S", 3 == state.page, ImVec2(234, 50), text_font,
                    icon_font)) {
        state.page = 3;
      }
      if (TabButton("Config", "C", 4 == state.page, ImVec2(234, 50), text_font,
                    icon_font)) {
        state.page = 4;
      }
      if (TabButton("Extras", "E", 5 == state.page, ImVec2(234, 50), text_font,
                    icon_font)) {
        state.page = 5;
      }

      ImGui::PopStyleVar();
    }
    ImGui::EndGroup();

    ImVec2 panel_size = ImGui::GetWindowSize();
    ImGui::SetCursorPos(ImVec2(left_inset, panel_size.y - (panel_gap + button_height)));
    ImFont *text_font = PickFont(assets.inter_s, ImGui::GetFont());
    ImFont *icon_font = PickFont(assets.icon, ImGui::GetFont());
    const float action_width = menu_width;
    if (TabButton("Close", "X", false, ImVec2(action_width, 50), text_font, icon_font)) {
      if (app_running) {
        *app_running = false;
      } else if (ui_visible) {
        *ui_visible = false;
      }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, state.tab_alpha * ImGui::GetStyle().Alpha);
    {
      state.anim_text = ImLerp(state.anim_text,
                               state.page == state.active_tab ? 20.0f : 0.0f,
                               14.0f * io.DeltaTime);

      ImFont *header_font = PickFont(assets.inter_s2, ImGui::GetFont());
      ImFont *base_font = PickFont(assets.inter_s, ImGui::GetFont());

      draw->AddText(header_font, 23.0f,
            ImVec2(p.x + content_left + state.anim_text, p.y + 18),
            ImGui::GetColorU32(colors.text_active),
            state.active_tab == 0 ? "Aimbot" :
            state.active_tab == 1 ? "Clickity" :
            state.active_tab == 2 ? "Visuals" :
            state.active_tab == 3 ? "Settings" :
            state.active_tab == 4 ? "Config" : "Extras");

      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

      const float content_width = region.x - content_left - content_right_inset;
      ImGui::SetCursorPos(ImVec2(content_left, 76));
      ImGui::BeginChild("tab_main", ImVec2(content_width, 520), false);
      {
        if (ImGui::BeginTable("tab_layout", 2,
                              ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX)) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          if (state.active_tab == 0) { // Aimbot
            ImGui::TextUnformatted("Aimbot");
            ui_widgets::Checkbox("Enable Aimbot", &g_aimbot_enabled);

            ImGui::Spacing();
            ImGui::TextDisabled("Activation");
            const char* mode_items[] = { "Hold", "Toggle" };
            ImGui::SetNextItemWidth(180.0f);
            ui_widgets::Combo("Mode", &g_aimbot_key_mode, mode_items, IM_ARRAYSIZE(mode_items), -1, nullptr);

            const char* hotkey_items[] = { "Mouse4", "Mouse5", "Space", "C", "V", "X", "Z", "F", "E", "Q" };
            int hotkey_index = 0;
            for (size_t i = 0; i < functionalities::kAimbotHotkeys.size(); ++i) {
              if (g_aimbot_hotkey == static_cast<int>(functionalities::kAimbotHotkeys[i].key)) {
                hotkey_index = static_cast<int>(i);
                break;
              }
            }
            ImGui::SetNextItemWidth(180.0f);
            if (ui_widgets::Combo("Hotkey", &hotkey_index, hotkey_items, IM_ARRAYSIZE(hotkey_items), -1, nullptr)) {
              g_aimbot_hotkey = static_cast<int>(functionalities::kAimbotHotkeys[hotkey_index].key);
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Tuning");
            ui_widgets::SliderFloat("FOV", &g_aimbot_fov, 0.5f, 30.0f, "%.1f");
            ui_widgets::SliderFloat("Smooth", &g_aimbot_smooth, 0.0f, 20.0f, "%.1f");

            float start_bullet = static_cast<float>(g_aimbot_start_bullet);
            if (ui_widgets::SliderFloat("Start Bullet", &start_bullet, 0.0f, 10.0f, "%.0f")) {
              g_aimbot_start_bullet = static_cast<int>(start_bullet);
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Checks");
            ui_widgets::Checkbox("Distance Adjusted FOV", &g_aimbot_distance_adjusted_fov);
            ui_widgets::Checkbox("Visibility Check", &g_aimbot_visibility_check);
            ui_widgets::Checkbox("Flash Check", &g_aimbot_flash_check);
            ui_widgets::Checkbox("Target Friendlies", &g_aimbot_target_friendlies);
            ui_widgets::Checkbox("Head Only", &g_aimbot_head_only);
          } else if (state.active_tab == 1) { // Clickity
            ui_widgets::Checkbox("Enable Triggerbot", &functionalities::Clickity::Get().enabled_);
            
            // Add configuration options for Clickity
            // We need to access the singleton instance's members, but we can't directly reference them 
            // easily without exposing them or adding friend. 
            // In clickity.h they are public, so we can access them.
            auto& clickity = functionalities::Clickity::Get();
            
            ImGui::Spacing();
            ImGui::TextDisabled("Checks");
            ui_widgets::Checkbox("Team Check", &clickity.team_check_);
            ui_widgets::Checkbox("Flash Check", &clickity.flash_check_);
            ui_widgets::Checkbox("Scope Check", &clickity.scope_check_);
            ui_widgets::Checkbox("Velocity Check", &clickity.velocity_check_);
            
            ImGui::Spacing();
            ImGui::TextDisabled("Delays (ms)");
            // We need integer sliders. The existing SliderFloat is float.
            // Let's use float slider but cast, or just use DragInt if we had it.
            // We'll cast for now since we only have SliderFloat in widgets.h/cpp
            float start = (float)clickity.delay_start_;
            if (ui_widgets::SliderFloat("Delay Start", &start, 0.0f, 200.0f, "%.0f")) clickity.delay_start_ = (int)start;
            
            float end = (float)clickity.delay_end_;
            if (ui_widgets::SliderFloat("Delay End", &end, 0.0f, 200.0f, "%.0f")) clickity.delay_end_ = (int)end;
            
            float duration = (float)clickity.shot_duration_;
            if (ui_widgets::SliderFloat("Shot Duration", &duration, 10.0f, 200.0f, "%.0f")) clickity.shot_duration_ = (int)duration;

            float click_delay = (float)clickity.click_delay_;
            if (ui_widgets::SliderFloat("Click Delay", &click_delay, 0.0f, 300.0f, "%.0f")) clickity.click_delay_ = (int)click_delay;

          } else if (state.active_tab == 2) { // Visuals
            ImGui::TextUnformatted("See-through walls");
            ui_widgets::Checkbox("Enable", &g_see_through_walls);

            ImGui::Spacing();
            ImGui::TextUnformatted("ESP Mode");
            const char* esp_mode_items[] = { "Basic", "Fake Chams" };
            ImGui::SetNextItemWidth(180.0f);
            if (ui_widgets::Combo("##esp_mode", &g_esp_mode, esp_mode_items,
                                  IM_ARRAYSIZE(esp_mode_items), -1, nullptr)) {
              if (g_esp_mode == static_cast<int>(EspRenderMode::Basic)) {
                g_esp_show_box = true;
                g_esp_show_skeleton = true;
                g_esp_show_health = true;
                g_esp_show_name = true;
                g_esp_show_weapon = true;
                g_esp_show_head = true;
                g_esp_fake_chams = false;
              } else if (g_esp_mode == static_cast<int>(EspRenderMode::FakeChams)) {
                g_esp_show_box = false;
                g_esp_show_skeleton = false;
                g_esp_show_health = false;
                g_esp_show_name = true;
                g_esp_show_weapon = true;
                g_esp_show_head = false;
                g_esp_fake_chams = true;
              }
            }

            ImGui::Spacing();
            ImGui::TextUnformatted("ESP Elements");
            ui_widgets::Checkbox("Box", &g_esp_show_box);
            ui_widgets::Checkbox("Skeleton", &g_esp_show_skeleton);
            ui_widgets::Checkbox("Health Bar", &g_esp_show_health);
            ui_widgets::Checkbox("Name", &g_esp_show_name);
            ui_widgets::Checkbox("Weapon", &g_esp_show_weapon);
            ui_widgets::Checkbox("Head Circle", &g_esp_show_head);
            ui_widgets::Checkbox("Fake Chams", &g_esp_fake_chams);
            
            ImGui::Spacing();
            ImGui::TextUnformatted("Team Filter");
            const char* team_items[] = { "Enemies", "Team", "All" };
            ImGui::SetNextItemWidth(180.0f);
            ui_widgets::Combo("##team_filter", &g_esp_team_filter, team_items, IM_ARRAYSIZE(team_items), -1, nullptr);


           } else if (state.active_tab == 3) { // Settings
             ImGui::TextUnformatted("Watermark");
             ui_widgets::Checkbox("Enable##watermark_toggle", &g_watermark_enabled);
          } else if (state.active_tab == 4) { // Config
            static char config_path[128] = "configs/default.cfg";
            static char status[128] = "";

            ImGui::TextUnformatted("Config file");
            ui_widgets::InputText("##config_path", config_path, sizeof(config_path));

            ImGui::Spacing();
            if (ImGui::Button("Save")) {
              if (config::SaveConfig(config_path)) {
                std::snprintf(status, sizeof(status), "Saved: %s", config_path);
              } else {
                std::snprintf(status, sizeof(status), "Save failed: %s", config_path);
              }
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
              if (config::LoadConfig(config_path)) {
                std::snprintf(status, sizeof(status), "Loaded: %s", config_path);
              } else {
                std::snprintf(status, sizeof(status), "Load failed: %s", config_path);
              }
            }

            if (status[0] != '\0') {
              ImGui::Spacing();
              ImGui::TextUnformatted(status);
            }
           } else if (state.active_tab == 5) { // Extras
            ImGui::TextUnformatted("Sniper Crosshair");
            ui_widgets::Checkbox("Enable##crosshair", &g_sniper_crosshair);

            ImGui::Spacing();
            ImGui::TextUnformatted("Headshoot Line");
            ui_widgets::Checkbox("Enable##headshoot_line", &g_headshoot_line);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::ColorEdit4("Color##headshoot_line", &g_headshoot_line_color.x,
                              ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            ImGui::Spacing();
            ImGui::TextUnformatted("Where Is Planted");
            ui_widgets::Checkbox("Enable##where_is_planted", &g_where_is_planted);

            ImGui::Spacing();
            ImGui::TextUnformatted("Grenade Prediction");
            ui_widgets::Checkbox("Enable##grenade_pred", &g_grenade_pred);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::ColorEdit4("Color##grenade_pred", &g_grenade_pred_color.x,
                              ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            ImGui::Spacing();
            ImGui::TextUnformatted("Grenade Proximity Warning");
            ui_widgets::Checkbox("Enable##grenade_prox", &g_grenade_proximity_warning);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::ColorEdit4("Zone Color##grenade_prox", &g_grenade_proximity_color.x,
                              ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::SetNextItemWidth(200.0f);
            ImGui::ColorEdit4("Fire Color##grenade_prox", &g_grenade_proximity_fire_color.x,
                              ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextUnformatted("Radar");
            ui_widgets::Checkbox("Enable Radar", &g_radar_enabled);
            ui_widgets::SliderFloat("Range", &g_radar_range, 500.0f, 5000.0f, "%.0f");
            ui_widgets::SliderFloat("Size", &g_radar_size, 100.0f, 400.0f, "%.0f");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextUnformatted("MVP Music");
            ui_widgets::Checkbox("Enable##mvp_music", &g_mvp_music_enabled);

            ImGui::Spacing();
            ImGui::TextUnformatted("Music");
            const char* music_items[] = { "Shakira - Latinas", "EZ4Ence" };
            ImGui::SetNextItemWidth(220.0f);
            ui_widgets::Combo("##mvp_music_select", &g_mvp_music_selection,
                        music_items, IM_ARRAYSIZE(music_items), -1, nullptr);

#ifdef CSIGA2_FUN
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextUnformatted("Random Video");
            ui_widgets::Checkbox("Enable##random_video", &g_video_random_enabled);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextUnformatted("Virtual Mic");

            static std::vector<audio::MicDevice> mic_devices;
            static int mic_index = 0;

            if (mic_devices.empty()) {
              mic_devices = audio::ListPhysicalMics();
              mic_index = 0;
            }

            if (ImGui::Button("Refresh Mics")) {
              mic_devices = audio::ListPhysicalMics();
              mic_index = 0;
            }

            if (!mic_devices.empty()) {
              std::vector<const char *> labels;
              labels.reserve(mic_devices.size());
              for (const auto &mic : mic_devices) {
                labels.push_back(mic.name.c_str());
              }

              ImGui::SetNextItemWidth(260.0f);
              if (ImGui::Combo("##mic_select", &mic_index, labels.data(),
                               static_cast<int>(labels.size()))) {
                audio::SetPhysicalMic(mic_devices[mic_index].name);
              }
            } else {
              ImGui::TextUnformatted("No microphones found.");
            }
#endif
          }

          ImGui::TableSetColumnIndex(1);
          ImGui::EndTable();
        }
      }
      ImGui::EndChild();

      ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
  }
  ImGui::End();
}
