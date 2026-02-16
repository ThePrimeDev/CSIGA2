#pragma once

#include <imgui.h>

namespace csiga2_style {

struct Colors {
  ImVec4 main_color = ImColor(255, 140, 0, 255);
  ImVec4 text_active = ImColor(255, 255, 255, 255);
  ImVec4 text_inactive = ImColor(255, 255, 255, 127);
  ImVec4 child_rect = ImColor(0, 0, 0, 140);
  ImVec4 child_bg = ImColor(12, 12, 13, 150);
  ImVec4 window_bg = ImColor(7, 7, 7, 255);
  ImVec4 popup_bg = ImColor(7, 8, 18, 127);
  ImVec4 tab_active = ImColor(11, 11, 11, 255);
  ImVec4 tab_inactive = ImColor(0, 0, 0, 255);
  ImVec4 input_bg = ImColor(11, 11, 11, 255);
  ImVec4 stroke = ImColor(28, 26, 37);
  ImVec4 background_slider = ImColor(9, 9, 9, 255);
  ImVec4 window = ImColor(7, 7, 7, 255);
  ImVec4 checkbox_bg_inactive = ImColor(9, 9, 9, 255);
  ImVec4 black = ImColor(0, 0, 0, 255);
};

Colors &GetColors();
void ApplyOstinTheme();

}  // namespace csiga2_style
