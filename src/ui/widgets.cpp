#define IMGUI_DEFINE_MATH_OPERATORS

#include "widgets.h"

#include "ostin_style.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdio>
#include <map>

namespace {

struct CheckboxState {
  ImVec4 background;
  ImVec4 circle;
  float move = 0.0f;
};

struct ComboState {
  ImVec4 text;
  float arrow_roll = 1.57f;
  float combo_size = 0.0f;
  bool opened_combo = false;
  bool hovered = false;
  float expand_rotation = 1.0f;
  float anim_alpha = 0.0f;
  float anim_alpha2 = 0.0f;
};

static std::map<ImGuiID, ComboState> g_combo_anim;

static void FormatHexColor(const float col[4], char *out, size_t out_size) {
  if (out_size == 0) {
    return;
  }
  int r = static_cast<int>(col[0] * 255.0f + 0.5f);
  int g = static_cast<int>(col[1] * 255.0f + 0.5f);
  int b = static_cast<int>(col[2] * 255.0f + 0.5f);
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  std::snprintf(out, out_size, "#%02X%02X%02X", r, g, b);
}

static ImVec4 TintColor(const ImVec4 &c, float delta) {
  return ImVec4(ImClamp(c.x + delta, 0.0f, 1.0f),
                ImClamp(c.y + delta, 0.0f, 1.0f),
                ImClamp(c.z + delta, 0.0f, 1.0f),
                c.w);
}

static float CalcMaxPopupHeightFromItemCount(int items_count) {
  ImGuiContext &g = *GImGui;
  if (items_count <= 0) {
    return FLT_MAX;
  }
  return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y +
         (g.Style.WindowPadding.y * 2.0f);
}

static int rotation_start_index = 0;

static void ImRotateStart() {
  rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
}

static ImVec2 ImRotationCenter() {
  ImVec2 l(FLT_MAX, FLT_MAX);
  ImVec2 u(-FLT_MAX, -FLT_MAX);
  const auto &buf = ImGui::GetWindowDrawList()->VtxBuffer;
  for (int i = rotation_start_index; i < buf.Size; i++) {
    l = ImMin(l, buf[i].pos);
    u = ImMax(u, buf[i].pos);
  }
  return ImVec2((l.x + u.x) * 0.5f, (l.y + u.y) * 0.5f);
}

static void ImRotateEnd(float rad, ImVec2 center) {
  float s = sinf(rad);
  float c = cosf(rad);
  center = ImRotate(center, s, c) - center;

  auto &buf = ImGui::GetWindowDrawList()->VtxBuffer;
  for (int i = rotation_start_index; i < buf.Size; i++) {
    buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
  }
}

static bool BeginComboInternal(const char *label,
                               const char *preview_value,
                               float items_count,
                               bool close_on_click,
                               ImFont *arrow_font,
                               ImGuiComboFlags flags) {
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  (void)arrow_font;
  (void)close_on_click;

  ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.HasFlags;
  g.NextWindowData.ClearFlags();

  if (window->SkipItems) {
    g.NextWindowData.HasFlags = backup_next_window_data_flags;
    return false;
  }

  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);

  IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) !=
            (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview));

  const float field_width = 170.0f;
  const float field_height = 28.0f;
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const float w = ImGui::GetContentRegionMax().x - style.WindowPadding.x;
  const ImVec2 pos = window->DC.CursorPos;
  const float row_height = ImMax(label_size.y, field_height) + 8.0f;

  const ImRect bb(ImVec2(pos.x + w - field_width, pos.y + (row_height - field_height) * 0.5f),
                  ImVec2(pos.x + w, pos.y + (row_height - field_height) * 0.5f + field_height));
  const ImRect total_bb(pos, ImVec2(pos.x + w, pos.y + row_height));
  ImGui::ItemSize(ImRect(total_bb.Min, total_bb.Max));
  if (!ImGui::ItemAdd(total_bb, id, &bb)) {
    g.NextWindowData.HasFlags = backup_next_window_data_flags;
    return false;
  }

  bool hovered = false;
  bool held = false;
  bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

  auto it_anim = g_combo_anim.find(id);
  if (it_anim == g_combo_anim.end()) {
    it_anim = g_combo_anim.insert({id, ComboState()}).first;
  }

  // Check if mouse is hovering the popup area (which hasn't been drawn yet, but we know where it will be)
  ImRect popup_rect(ImVec2(bb.Min.x, bb.Max.y + 4.0f), 
                    ImVec2(bb.Min.x + bb.GetWidth(), bb.Max.y + 4.0f + it_anim->second.combo_size));
  bool hover_popup = it_anim->second.opened_combo && popup_rect.Contains(g.IO.MousePos);

  if ((hovered && g.IO.MouseClicked[0]) ||
      (it_anim->second.opened_combo && g.IO.MouseClicked[0] && !it_anim->second.hovered && !hover_popup)) {
    it_anim->second.opened_combo = !it_anim->second.opened_combo;
  }

  const csiga2_style::Colors &colors = csiga2_style::GetColors();
  it_anim->second.combo_size =
      ImLerp(it_anim->second.combo_size, it_anim->second.opened_combo ? (items_count * 35.0f) : 0.0f,
             g.IO.DeltaTime * 14.0f);
  it_anim->second.arrow_roll =
      ImLerp(it_anim->second.arrow_roll, it_anim->second.opened_combo ? -1.0f : 1.0f,
             g.IO.DeltaTime * 14.0f);
  it_anim->second.text =
      ImLerp(it_anim->second.text, it_anim->second.opened_combo ? colors.text_active : colors.text_inactive,
             g.IO.DeltaTime * 14.0f);
  it_anim->second.expand_rotation =
      ImLerp(it_anim->second.expand_rotation, it_anim->second.opened_combo ? -1.0f : 1.0f,
             g.IO.DeltaTime * 10.0f);

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(colors.input_bg), 5.0f);
  draw_list->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(colors.stroke), 5.0f);

  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(it_anim->second.text));
  ImGui::RenderTextClipped(bb.Min + ImVec2(10.0f, 6.0f), ImVec2(bb.Max.x - 20.0f, bb.Max.y),
                           preview_value ? preview_value : "", nullptr, nullptr);
  ImGui::PopStyleColor();

  ImVec2 label_pos(pos.x + 16.0f, pos.y + (row_height - label_size.y) * 0.5f);
  draw_list->AddText(label_pos, ImGui::GetColorU32(colors.text_active), label);

  ImVec2 arrow_center(bb.Max.x - 12.0f, (bb.Min.y + bb.Max.y) * 0.5f);
  
  // Simple arrow (triangle)
  if (it_anim->second.opened_combo) {
      draw_list->AddTriangleFilled(
          ImVec2(arrow_center.x - 3.5f, arrow_center.y + 2.0f),
          ImVec2(arrow_center.x + 3.5f, arrow_center.y + 2.0f),
          ImVec2(arrow_center.x, arrow_center.y - 2.0f),
          ImGui::GetColorU32(colors.text_inactive)
      );
  } else {
      draw_list->AddTriangleFilled(
          ImVec2(arrow_center.x - 3.5f, arrow_center.y - 2.0f),
          ImVec2(arrow_center.x + 3.5f, arrow_center.y - 2.0f),
          ImVec2(arrow_center.x, arrow_center.y + 2.0f),
          ImGui::GetColorU32(colors.text_inactive)
      );
  }

  if (!ImGui::IsRectVisible(bb.Min, bb.Max + ImVec2(0.0f, 2.0f))) {
    it_anim->second.opened_combo = false;
    it_anim->second.combo_size = 0.0f;
  }
  it_anim->second.anim_alpha =
      ImLerp(it_anim->second.anim_alpha, hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
  it_anim->second.anim_alpha2 =
      ImLerp(it_anim->second.anim_alpha2, hovered ? 0.75f : 0.0f, g.IO.DeltaTime * 10.0f);

  if (!it_anim->second.opened_combo && it_anim->second.combo_size < 2.0f) {
    g.NextWindowData.HasFlags = backup_next_window_data_flags;
    return false;
  }

  ImGui::SetNextWindowPos(ImVec2(bb.Min.x, bb.Max.y + 4.0f));
  ImGui::SetNextWindowSize(ImVec2(bb.GetWidth(), it_anim->second.combo_size));

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollWithMouse;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, colors.input_bg);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 4.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  bool ret = ImGui::Begin(label, nullptr, window_flags);
  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor();

  it_anim->second.hovered = ImGui::IsWindowHovered();

  g.NextWindowData.HasFlags = backup_next_window_data_flags;
  return ret;
}

}  // namespace

namespace ui_widgets {

bool Checkbox(const char *label, bool *value) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const float toggle_height = 18.0f;
  const float toggle_width = 36.0f;
  const float w = ImGui::GetContentRegionMax().x - style.WindowPadding.x;
  const float row_height = ImMax(label_size.y, toggle_height) + 8.0f;

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(pos, ImVec2(pos.x + w, pos.y + row_height));
  ImGui::ItemSize(total_bb, 0.0f);
  if (!ImGui::ItemAdd(total_bb, id)) {
    return false;
  }

  bool hovered = false;
  bool held = false;
  bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);

  if (pressed) {
    *value = !(*value);
    ImGui::MarkItemEdited(id);
  }

  const csiga2_style::Colors &colors = csiga2_style::GetColors();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  static std::map<ImGuiID, CheckboxState> anim;
  CheckboxState &anim_state = anim[id];
  anim_state.move = ImLerp(anim_state.move, *value ? 1.0f : 0.0f, g.IO.DeltaTime * 12.0f);

  ImVec2 label_pos(pos.x + 16.0f, pos.y + (row_height - label_size.y) * 0.5f);
  ImU32 text_col = ImGui::GetColorU32(*value ? colors.text_active : colors.text_inactive);
  draw_list->AddText(label_pos, text_col, label);

  ImVec2 toggle_min(pos.x + w - toggle_width, pos.y + (row_height - toggle_height) * 0.5f);
  ImVec2 toggle_max(toggle_min.x + toggle_width, toggle_min.y + toggle_height);
  ImVec4 bg = ImLerp(colors.input_bg, colors.main_color, anim_state.move);
  ImU32 border = ImGui::GetColorU32(colors.stroke);
  draw_list->AddRectFilled(toggle_min, toggle_max, ImGui::GetColorU32(bg), toggle_height * 0.5f);
  draw_list->AddRect(toggle_min, toggle_max, border, toggle_height * 0.5f);

  const float knob_radius = toggle_height * 0.5f - 2.0f;
  const float knob_x = toggle_min.x + knob_radius + 2.0f +
                       (toggle_width - toggle_height) * anim_state.move;
  const float knob_y = toggle_min.y + toggle_height * 0.5f;
  draw_list->AddCircleFilled(ImVec2(knob_x, knob_y), knob_radius,
                             ImGui::GetColorU32(colors.text_active), 32);

  return pressed;
}

bool Combo(const char *label,
           int *current_item,
           const char *const items[],
           int items_count,
           int height_in_items,
           ImFont *arrow_font) {
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = ImGui::GetCurrentWindow();

  const char *preview_value = nullptr;
  if (*current_item >= 0 && *current_item < items_count) {
    preview_value = items[*current_item];
  }

  if (height_in_items != -1 && !(g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint)) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f),
                                        ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(height_in_items)));
  }

  if (!BeginComboInternal(label, preview_value, static_cast<float>(items_count), true, arrow_font, ImGuiComboFlags_None)) {
    return false;
  }

  bool value_changed = false;
  for (int i = 0; i < items_count; i++) {
    ImGui::PushID(i);
    const bool item_selected = (i == *current_item);
    const char *item_text = items[i] ? items[i] : "*Unknown item*";
    if (ImGui::Selectable(item_text, item_selected)) {
      value_changed = true;
      *current_item = i;
    }
    if (item_selected) {
      ImGui::SetItemDefaultFocus();
    }
    ImGui::PopID();
  }

  ImGui::End();

  if (value_changed) {
    ImGuiID id = window->GetID(label);
    auto it_anim = g_combo_anim.find(id);
    if (it_anim != g_combo_anim.end()) {
      it_anim->second.opened_combo = false;
    }
    ImGui::MarkItemEdited(g.LastItemData.ID);
  }

  return value_changed;
}

bool SliderFloat(const char *label,
                 float *value,
                 float min_value,
                 float max_value,
                 const char *format) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const float w = ImGui::GetContentRegionMax().x - style.WindowPadding.x;

  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const ImVec2 pos = window->DC.CursorPos;
  const float track_height = 4.0f;
  const float track_y = pos.y + label_size.y + 8.0f;
  const ImVec2 track_min(pos.x + 16.0f, track_y);
  const ImVec2 track_max(pos.x + w - 16.0f, track_y + track_height);
  const ImRect frame_bb(track_min, track_max);
  const ImRect total_bb(pos, ImVec2(pos.x + w, track_max.y + 10.0f));

  ImGui::ItemSize(total_bb, 0.0f);
  if (!ImGui::ItemAdd(total_bb, id, &frame_bb)) {
    return false;
  }

  const bool hovered = ImGui::ItemHoverable(frame_bb, id, ImGuiItemFlags_None);

  bool temp_input_is_active = false;
  if (!temp_input_is_active) {
    bool clicked = hovered && ImGui::IsMouseClicked(0);
    bool make_active = clicked;
    if (make_active && clicked) {
      if (clicked && g.IO.KeyCtrl) {
        temp_input_is_active = true;
      }
    }

    if (make_active && !temp_input_is_active) {
      ImGui::SetActiveID(id, window);
      ImGui::SetFocusID(id, window);
      ImGui::FocusWindow(window);
      g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }
  }

  if (temp_input_is_active) {
    return ImGui::TempInputScalar(frame_bb,
                                  id,
                                  label,
                                  ImGuiDataType_Float,
                                  value,
                                  format ? format : "%.3f",
                                  nullptr,
                                  nullptr);
  }

  ImRect grab_bb;

  struct SliderState {
    ImVec4 text;
    float slow_anim = 0.0f;
    float anim_alpha = 0.0f;
    float anim_alpha2 = 0.0f;
  };

  static std::map<ImGuiID, SliderState> anim;
  auto it_anim = anim.find(id);
  if (it_anim == anim.end()) {
    it_anim = anim.insert({id, SliderState()}).first;
  }

  const csiga2_style::Colors &colors = csiga2_style::GetColors();
  const bool active = ImGui::IsItemActive();
  it_anim->second.text =
      ImLerp(it_anim->second.text, active ? colors.text_active : colors.text_inactive,
             g.IO.DeltaTime * 14.0f);

  const bool value_changed = ImGui::SliderBehavior(frame_bb,
                                                   id,
                                                   ImGuiDataType_Float,
                                                   value,
                                                   &min_value,
                                                   &max_value,
                                                   format ? format : "%.3f",
                                                   ImGuiSliderFlags_None,
                                                   &grab_bb);
  if (value_changed) {
    ImGui::MarkItemEdited(id);
  }

  char value_buf[64];
  const char *value_buf_end =
      value_buf + ImGui::DataTypeFormatString(value_buf,
                                               IM_ARRAYSIZE(value_buf),
                                               ImGuiDataType_Float,
                                               value,
                                               format ? format : "%.3f");

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  draw_list->AddRectFilled(frame_bb.Min, frame_bb.Max,
                           ImGui::GetColorU32(colors.black),
                           track_height * 0.5f);

  if (grab_bb.Max.x > grab_bb.Min.x) {
    it_anim->second.slow_anim =
        ImLerp(it_anim->second.slow_anim, grab_bb.Min.x - frame_bb.Min.x, g.IO.DeltaTime * 25.0f);
    float fill_x = ImClamp(frame_bb.Min.x + it_anim->second.slow_anim, frame_bb.Min.x, frame_bb.Max.x);
    ImU32 left = ImGui::GetColorU32(colors.black);
    ImU32 right = ImGui::GetColorU32(colors.main_color);
    draw_list->AddRectFilledMultiColor(frame_bb.Min,
                                       ImVec2(fill_x, frame_bb.Max.y),
                                       left,
                                       right,
                                       right,
                                       left);
  }

  ImU32 label_col = ImGui::GetColorU32((hovered || active) ? colors.text_active : colors.text_inactive);
  ImU32 value_col = ImGui::GetColorU32((hovered || active) ? colors.main_color : colors.text_inactive);

  ImVec2 label_pos(pos.x + 16.0f, pos.y);
  draw_list->AddText(label_pos, label_col, label);

  ImVec2 value_size = ImGui::CalcTextSize(value_buf, value_buf_end);
  ImVec2 value_pos(pos.x + w - value_size.x, pos.y);
  draw_list->AddText(value_pos, value_col, value_buf);

  it_anim->second.anim_alpha =
      ImLerp(it_anim->second.anim_alpha, hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
  it_anim->second.anim_alpha2 =
      ImLerp(it_anim->second.anim_alpha2, hovered ? 0.75f : 0.0f, g.IO.DeltaTime * 10.0f);

  return value_changed;
}

bool InputText(const char *label, char *buf, size_t buf_size) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const float w = ImGui::GetContentRegionMax().x - style.WindowPadding.x;
  const float field_width = 170.0f;
  const float field_height = 28.0f;
  const float row_height = ImMax(label_size.y, field_height) + 8.0f;

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(pos, ImVec2(pos.x + w, pos.y + row_height));
  ImGui::ItemSize(total_bb, 0.0f);
  if (!ImGui::ItemAdd(total_bb, id)) {
    return false;
  }

  const csiga2_style::Colors &colors = csiga2_style::GetColors();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  ImVec2 label_pos(pos.x + 16.0f, pos.y + (row_height - label_size.y) * 0.5f);
  draw_list->AddText(label_pos, ImGui::GetColorU32(colors.text_active), label);

  ImVec2 field_min(pos.x + w - field_width, pos.y + (row_height - field_height) * 0.5f);
  ImVec2 field_max(field_min.x + field_width, field_min.y + field_height);

  ImGui::PushID(label);
  ImGui::SetCursorScreenPos(field_min);
  ImGui::PushItemWidth(field_width);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(colors.input_bg));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetColorU32(colors.input_bg));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::GetColorU32(colors.input_bg));
  ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(colors.text_active));

  bool changed = ImGui::InputText("##input", buf, buf_size);

  ImGui::PopStyleColor(4);
  ImGui::PopStyleVar(2);
  ImGui::PopItemWidth();

  const bool active = ImGui::IsItemActive();
  ImU32 border_col = ImGui::GetColorU32(colors.stroke);
  ImU32 line_col = ImGui::GetColorU32(active ? colors.main_color : colors.stroke);
  draw_list->AddRect(field_min, field_max, border_col, 5.0f);
  draw_list->AddLine(ImVec2(field_min.x + 8.0f, field_max.y - 2.0f),
                     ImVec2(field_max.x - 8.0f, field_max.y - 2.0f),
                     line_col, 1.5f);

  ImGui::PopID();
  return changed;
}

bool DotSlider(const char *label,
               float *value,
               float min_value,
               float max_value,
               const char *format) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const float w = ImGui::GetContentRegionMax().x - style.WindowPadding.x;
  const float knob_size = 28.0f;
  const float row_height = ImMax(label_size.y, knob_size) + 8.0f;

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(pos, ImVec2(pos.x + w, pos.y + row_height));
  ImGui::ItemSize(total_bb, 0.0f);
  if (!ImGui::ItemAdd(total_bb, id)) {
    return false;
  }

  const csiga2_style::Colors &colors = csiga2_style::GetColors();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  ImVec2 label_pos(pos.x + 16.0f, pos.y + (row_height - label_size.y) * 0.5f);
  draw_list->AddText(label_pos, ImGui::GetColorU32(colors.text_active), label);

  char value_buf[64];
  const char *value_buf_end =
      value_buf + ImGui::DataTypeFormatString(value_buf,
                                               IM_ARRAYSIZE(value_buf),
                                               ImGuiDataType_Float,
                                               value,
                                               format ? format : "%.2f");

  const ImVec2 knob_min(pos.x + w - knob_size, pos.y + (row_height - knob_size) * 0.5f);
  const ImVec2 knob_max(knob_min.x + knob_size, knob_min.y + knob_size);
  const ImRect knob_bb(knob_min, knob_max);

  ImVec2 value_size = ImGui::CalcTextSize(value_buf, value_buf_end);
  ImVec2 value_pos(knob_min.x - 10.0f - value_size.x, pos.y + (row_height - value_size.y) * 0.5f);
  draw_list->AddText(value_pos, ImGui::GetColorU32(colors.text_inactive), value_buf);

  ImGui::PushID(label);
  ImGui::SetCursorScreenPos(knob_min);
  ImGui::InvisibleButton("##dot", ImVec2(knob_size, knob_size));
  bool active = ImGui::IsItemActive();
  bool value_changed = false;

  if (active) {
    float speed = (max_value - min_value) / 180.0f;
    *value = ImClamp(*value + g.IO.MouseDelta.x * speed, min_value, max_value);
    value_changed = true;
  }

  float t = (max_value > min_value) ? (*value - min_value) / (max_value - min_value) : 0.0f;
  t = ImClamp(t, 0.0f, 1.0f);

  ImVec2 center((knob_min.x + knob_max.x) * 0.5f, (knob_min.y + knob_max.y) * 0.5f);
  float radius = (knob_size * 0.5f) - 3.0f;
  float start_angle = 3.1415926f * 0.75f;
  float end_angle = 3.1415926f * 2.25f;
  float fill_angle = start_angle + (end_angle - start_angle) * t;

  draw_list->AddCircleFilled(center, radius + 2.0f, ImGui::GetColorU32(colors.input_bg), 32);
  draw_list->PathArcTo(center, radius, start_angle, end_angle, 24);
  draw_list->PathStroke(ImGui::GetColorU32(colors.stroke), 0, 2.0f);
  draw_list->PathArcTo(center, radius, start_angle, fill_angle, 24);
  draw_list->PathStroke(ImGui::GetColorU32(colors.main_color), 0, 2.0f);

  ImVec2 dot_pos(center.x + std::cos(fill_angle) * radius,
                 center.y + std::sin(fill_angle) * radius);
  draw_list->AddCircleFilled(dot_pos, 3.0f, ImGui::GetColorU32(colors.main_color), 12);
  draw_list->AddCircle(center, radius - 6.0f, ImGui::GetColorU32(colors.black), 24, 2.0f);

  ImGui::PopID();
  return value_changed;
}

bool KeyBind(const char *label, int *key, const char *empty_text) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const float w = ImGui::GetContentRegionMax().x - style.WindowPadding.x;
  const float field_width = 90.0f;
  const float field_height = 28.0f;
  const float row_height = ImMax(label_size.y, field_height) + 8.0f;

  const ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(pos, ImVec2(pos.x + w, pos.y + row_height));
  ImGui::ItemSize(total_bb, 0.0f);
  if (!ImGui::ItemAdd(total_bb, window->GetID(label))) {
    return false;
  }

  const csiga2_style::Colors &colors = csiga2_style::GetColors();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  ImVec2 label_pos(pos.x + 16.0f, pos.y + (row_height - label_size.y) * 0.5f);
  draw_list->AddText(label_pos, ImGui::GetColorU32(colors.text_active), label);

  ImVec2 field_min(pos.x + w - field_width, pos.y + (row_height - field_height) * 0.5f);
  ImVec2 field_max(field_min.x + field_width, field_min.y + field_height);
  ImRect field_bb(field_min, field_max);

  ImGui::PushID(label);
  ImGuiID field_id = window->GetID("##keybind");
  bool hovered = false;
  bool held = false;
  bool pressed = ImGui::ButtonBehavior(field_bb, field_id, &hovered, &held);

  struct KeyBindState {
    bool waiting = false;
  };
  static std::map<ImGuiID, KeyBindState> anim;
  auto it_anim = anim.find(field_id);
  if (it_anim == anim.end()) {
    it_anim = anim.insert({field_id, KeyBindState()}).first;
  }

  if (pressed) {
    it_anim->second.waiting = true;
  }

  if (it_anim->second.waiting) {
    for (int key_index = ImGuiKey_NamedKey_BEGIN; key_index < ImGuiKey_NamedKey_END; ++key_index) {
      ImGuiKey key_code = static_cast<ImGuiKey>(key_index);
      if (ImGui::IsKeyPressed(key_code)) {
        *key = key_index;
        it_anim->second.waiting = false;
        break;
      }
    }
    if (ImGui::IsMouseClicked(1)) {
      it_anim->second.waiting = false;
    }
  }

  draw_list->AddRectFilled(field_bb.Min, field_bb.Max, ImGui::GetColorU32(colors.input_bg), 5.0f);
  draw_list->AddRect(field_bb.Min, field_bb.Max, ImGui::GetColorU32(colors.stroke), 5.0f);

  const char *key_text = empty_text;
  if (it_anim->second.waiting) {
    key_text = "...";
  } else if (*key > 0) {
    const char *name = ImGui::GetKeyName(static_cast<ImGuiKey>(*key));
    if (name && name[0] != '\0') {
      key_text = name;
    }
  }

  ImVec2 key_size = ImGui::CalcTextSize(key_text);
  ImVec2 key_pos(field_min.x + (field_width - key_size.x) * 0.5f,
                 field_min.y + (field_height - key_size.y) * 0.5f);
  draw_list->AddText(key_pos, ImGui::GetColorU32(colors.text_active), key_text);

  ImGui::PopID();
  return pressed;
}

bool ColorEdit4(const char *label, float col[4], ImGuiColorEditFlags flags) {
  ImGuiWindow *window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return false;
  }

  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
  const float w = ImGui::GetContentRegionMax().x - style.WindowPadding.x;
  const float field_width = 190.0f;
  const float field_height = 28.0f;
  const float row_height = ImMax(label_size.y, field_height) + 8.0f;

  ImVec2 pos = window->DC.CursorPos;
  const ImRect total_bb(pos, ImVec2(pos.x + w, pos.y + row_height));
  ImGui::ItemSize(total_bb, 0.0f);
  if (!ImGui::ItemAdd(total_bb, id)) {
    return false;
  }

  const csiga2_style::Colors &colors = csiga2_style::GetColors();
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  ImVec2 label_pos(pos.x + 16.0f, pos.y + (row_height - label_size.y) * 0.5f);
  draw_list->AddText(label_pos, ImGui::GetColorU32(colors.text_active), label);

  ImVec2 field_min(pos.x + w - field_width, pos.y + (row_height - field_height) * 0.5f);
  ImVec2 field_max(field_min.x + field_width, field_min.y + field_height);
  ImRect field_bb(field_min, field_max);

  draw_list->AddRectFilled(field_bb.Min, field_bb.Max, ImGui::GetColorU32(colors.input_bg), 5.0f);
  draw_list->AddRect(field_bb.Min, field_bb.Max, ImGui::GetColorU32(colors.stroke), 5.0f);

  char hex_buf[16];
  FormatHexColor(col, hex_buf, sizeof(hex_buf));

  const ImVec4 base(col[0], col[1], col[2], 1.0f);
  const ImVec4 light = TintColor(base, 0.12f);
  const ImVec4 dark = TintColor(base, -0.12f);

  const float swatch_size = 12.0f;
  const float swatch_spacing = 3.0f;
  const float swatch_total = swatch_size * 3.0f + swatch_spacing * 2.0f;
  ImVec2 swatch_start(field_max.x - 8.0f - swatch_total, field_min.y + (field_height - swatch_size) * 0.5f);

  draw_list->AddRectFilled(swatch_start,
                           swatch_start + ImVec2(swatch_size, swatch_size),
                           ImGui::GetColorU32(base), 2.0f);
  draw_list->AddRectFilled(swatch_start + ImVec2(swatch_size + swatch_spacing, 0.0f),
                           swatch_start + ImVec2(swatch_size * 2.0f + swatch_spacing, swatch_size),
                           ImGui::GetColorU32(light), 2.0f);
  draw_list->AddRectFilled(swatch_start + ImVec2((swatch_size + swatch_spacing) * 2.0f, 0.0f),
                           swatch_start + ImVec2(swatch_total, swatch_size),
                           ImGui::GetColorU32(dark), 2.0f);

  ImVec2 hex_size = ImGui::CalcTextSize(hex_buf);
  ImVec2 hex_pos(swatch_start.x - 8.0f - hex_size.x, field_min.y + (field_height - hex_size.y) * 0.5f);
  draw_list->AddText(hex_pos, ImGui::GetColorU32(colors.text_inactive), hex_buf);

  ImGui::PushID(label);
  ImGui::SetCursorScreenPos(field_min);
  bool pressed = ImGui::InvisibleButton("##color", ImVec2(field_width, field_height));
  ImGui::PopID();

  bool value_changed = false;
  if (pressed && !(flags & ImGuiColorEditFlags_NoPicker)) {
    g.ColorPickerRef = ImVec4(col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
    ImGui::OpenPopup("picker");
    ImGui::SetNextWindowPos(field_bb.GetBL() + ImVec2(0.0f, style.ItemSpacing.y));
  }

  ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetColorU32(colors.window));
  ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(colors.window));
  ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));

  if (ImGui::BeginPopup("picker")) {
    if (g.CurrentWindow->BeginCount == 1) {
      ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_DisplayMask_ |
                                          ImGuiColorEditFlags_NoLabel |
                                          ImGuiColorEditFlags_NoAlpha |
                                          ImGuiColorEditFlags_AlphaBar |
                                          ImGuiColorEditFlags_AlphaPreviewHalf;
      ImGui::SetNextItemWidth(200.0f);
      value_changed |= ImGui::ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
    }
    ImGui::EndPopup();
  }

  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);

  return value_changed;
}

}  // namespace ui_widgets
