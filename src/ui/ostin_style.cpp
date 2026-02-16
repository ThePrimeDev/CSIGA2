#include "ostin_style.h"

namespace csiga2_style {

Colors &GetColors() {
  static Colors colors;
  return colors;
}

void ApplyOstinTheme() {
  ImGuiStyle &style = ImGui::GetStyle();
  Colors &colors = GetColors();

  style.WindowRounding = 12.0f;
  style.ChildRounding = 0.0f;
  style.WindowPadding = ImVec2(0.0f, 0.0f);
  style.FramePadding = ImVec2(10.0f, 6.0f);
  style.ItemSpacing = ImVec2(10.0f, 10.0f);
  style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
  style.WindowBorderSize = 0.0f;

  ImVec4 *col = style.Colors;
  col[ImGuiCol_Border] = ImVec4(0, 0, 0, 0);
  col[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.06f, 0.97f);
  col[ImGuiCol_ChildBg] = colors.child_bg;
  col[ImGuiCol_WindowBg] = colors.window_bg;

  // Color picker / popup styling
  col[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.0f);
  col[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
  col[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
  col[ImGuiCol_SliderGrab] = colors.main_color;
  col[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.65f, 0.15f, 1.0f);
  col[ImGuiCol_Button] = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
  col[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
  col[ImGuiCol_ButtonActive] = ImVec4(colors.main_color.x, colors.main_color.y, colors.main_color.z, 0.6f);
  col[ImGuiCol_Header] = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
  col[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
  col[ImGuiCol_HeaderActive] = ImVec4(colors.main_color.x, colors.main_color.y, colors.main_color.z, 0.4f);
  col[ImGuiCol_Text] = colors.text_active;
  col[ImGuiCol_TextDisabled] = colors.text_inactive;

  // Popup border for color pickers
  style.PopupRounding = 8.0f;
  style.PopupBorderSize = 1.0f;
  col[ImGuiCol_Border] = ImVec4(0.22f, 0.20f, 0.28f, 0.6f);
}

}  // namespace csiga2_style
