#include "see_trough_demo.h"

#include <cmath>

#include <imgui_internal.h>

namespace {

float SmoothNoise(float t, float scale, float phase) {
  return 0.5f + 0.5f * std::sin(t * scale + phase);
}

}  // namespace

void RenderSeeTroughDemo(const ImVec2 &top_left, const ImVec2 &size) {
  ImDrawList *draw = ImGui::GetWindowDrawList();
  const float t = static_cast<float>(ImGui::GetTime());

  const float pad = 24.0f;
  const ImVec2 area_min = ImVec2(top_left.x + pad, top_left.y + pad);
  const ImVec2 area_max = ImVec2(top_left.x + size.x - pad, top_left.y + size.y - pad);

  const float nx = SmoothNoise(t, 0.7f, 1.3f);
  const float ny = SmoothNoise(t, 0.9f, 2.1f);
  const float bob = SmoothNoise(t, 2.2f, 0.4f);

  const ImVec2 center = ImVec2(ImLerp(area_min.x, area_max.x, nx),
                               ImLerp(area_min.y, area_max.y, ny));

  const float body_h = 130.0f;
  const float body_w = 60.0f;
  const float head_r = 14.0f;

  const ImU32 col_main = ImGui::GetColorU32(ImVec4(1.0f, 0.55f, 0.0f, 1.0f));
  const ImU32 col_secondary = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.9f));
  const ImU32 col_back = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.35f));

  const ImVec2 box_min = ImVec2(center.x - body_w * 0.5f, center.y - body_h * 0.5f);
  const ImVec2 box_max = ImVec2(center.x + body_w * 0.5f, center.y + body_h * 0.5f);

  // ESP box
  draw->AddRect(box_min, box_max, col_main, 4.0f, 0, 2.0f);

  // Health bar
  const float hp = 0.35f + 0.65f * bob;
  const float bar_w = 6.0f;
  const ImVec2 bar_min = ImVec2(box_min.x - 10.0f, box_min.y);
  const ImVec2 bar_max = ImVec2(bar_min.x + bar_w, box_max.y);
  const float hp_h = (bar_max.y - bar_min.y) * hp;
  const ImVec4 hp_low = ImVec4(0.45f, 0.08f, 0.08f, 1.0f);
  const ImVec4 hp_high = ImVec4(0.10f, 0.45f, 0.10f, 1.0f);
  const ImU32 hp_col = ImGui::GetColorU32(ImLerp(hp_low, hp_high, hp));
  draw->AddRectFilled(bar_min, bar_max, col_back, 3.0f);
  draw->AddRect(bar_min, bar_max, col_secondary, 3.0f);
  draw->AddRectFilled(ImVec2(bar_min.x, bar_max.y - hp_h), bar_max, hp_col, 3.0f);

  // Skeleton
  enum BoneId {
    kHip,
    kSpine1,
    kSpine2,
    kSpine3,
    kSpine4,
    kNeck,
    kHead,
    kLeftShoulder,
    kLeftElbow,
    kLeftHand,
    kRightShoulder,
    kRightElbow,
    kRightHand,
    kLeftHip,
    kLeftKnee,
    kLeftFoot,
    kRightHip,
    kRightKnee,
    kRightFoot,
    kBoneCount
  };

  const int connections[][2] = {
      {kHip, kSpine1},       {kSpine1, kSpine2},   {kSpine2, kSpine3},
      {kSpine3, kSpine4},    {kSpine4, kNeck},     {kNeck, kHead},
      {kNeck, kLeftShoulder}, {kLeftShoulder, kLeftElbow},
      {kLeftElbow, kLeftHand}, {kNeck, kRightShoulder},
      {kRightShoulder, kRightElbow}, {kRightElbow, kRightHand},
      {kHip, kLeftHip},      {kLeftHip, kLeftKnee}, {kLeftKnee, kLeftFoot},
      {kHip, kRightHip},     {kRightHip, kRightKnee}, {kRightKnee, kRightFoot}};

  ImVec2 bones[kBoneCount];
  const float pad_inner = 6.0f;
  const ImVec2 clamp_min = ImVec2(box_min.x + pad_inner, box_min.y + pad_inner);
  const ImVec2 clamp_max = ImVec2(box_max.x - pad_inner, box_max.y - pad_inner);

  auto ClampToBox = [&](ImVec2 p) {
    p.x = ImClamp(p.x, clamp_min.x, clamp_max.x);
    p.y = ImClamp(p.y, clamp_min.y, clamp_max.y);
    return p;
  };

  const float cycle = std::fmod(t, 8.0f);
  const bool is_idle = cycle < 2.0f;
  const bool is_walk = cycle >= 2.0f && cycle < 5.0f;
  const bool is_crouch = cycle >= 5.0f && cycle < 6.5f;
  const bool is_aim = cycle >= 6.5f;

  const float walk_amp = is_walk ? 1.0f : 0.25f;
  const float walk = std::sin(t * 3.0f) * walk_amp;
  const float walk_opposite = std::sin(t * 3.0f + 3.14159f) * walk_amp;
  const float stride = std::sin(t * 1.5f) * walk_amp;

  const float crouch_target = is_crouch ? 1.0f : 0.0f;
  const float crouch = ImLerp(0.0f, 1.0f, crouch_target) + 0.15f * std::sin(t * 2.2f);
  const float crouch_amt = 0.55f * crouch;

  const float aim_target = is_aim ? 1.0f : 0.0f;
  const float aim = ImLerp(0.2f, 1.0f, aim_target);

  const float box_w = clamp_max.x - clamp_min.x;
  const float box_h = clamp_max.y - clamp_min.y;
  const float hip_y = clamp_min.y + box_h * (0.58f + 0.18f * crouch_amt) +
                      stride * box_h * 0.03f;
  const float spine_scale = 1.0f - crouch_amt;
  const float sway = std::sin(t * 1.1f) * (box_w * 0.06f);

  auto Map = [&](float nx, float ny) {
    return ClampToBox(ImVec2(clamp_min.x + box_w * nx, clamp_min.y + box_h * ny));
  };

  // Base spine chain
  bones[kHip] = ClampToBox(ImVec2(center.x + sway * 0.3f, hip_y));
  bones[kSpine1] = ClampToBox(ImVec2(bones[kHip].x + sway * 0.1f,
                                    hip_y - box_h * 0.10f * spine_scale));
  bones[kSpine2] = ClampToBox(ImVec2(bones[kHip].x + sway * 0.15f,
                                    hip_y - box_h * 0.20f * spine_scale));
  bones[kSpine3] = ClampToBox(ImVec2(bones[kHip].x + sway * 0.2f,
                                    hip_y - box_h * 0.30f * spine_scale));
  bones[kSpine4] = ClampToBox(ImVec2(bones[kHip].x + sway * 0.25f,
                                    hip_y - box_h * 0.40f * spine_scale));
  bones[kNeck] = ClampToBox(ImVec2(bones[kHip].x + sway * 0.3f,
                                  hip_y - box_h * 0.48f * spine_scale));
  bones[kHead] = ClampToBox(ImVec2(bones[kNeck].x + sway * 0.1f,
                                  bones[kNeck].y - box_h * 0.08f * spine_scale));

  // Shoulders
  const float shoulder_span = box_w * 0.24f;
  bones[kLeftShoulder] = ClampToBox(ImVec2(bones[kSpine4].x - shoulder_span,
                                          bones[kSpine4].y + box_h * 0.02f));
  bones[kRightShoulder] = ClampToBox(ImVec2(bones[kSpine4].x + shoulder_span,
                                           bones[kSpine4].y + box_h * 0.02f));

  // Arm pose: blend walking swing with weapon hold
  const float arm_swing = walk * (box_w * 0.10f) * (1.0f - aim * 0.7f);
  const float hold_forward = box_w * (0.12f + 0.06f * aim);
  const float hold_down = box_h * (0.02f + 0.02f * aim);

  bones[kLeftElbow] = ClampToBox(ImVec2(bones[kLeftShoulder].x - box_w * 0.12f - arm_swing * 0.6f,
                                       bones[kLeftShoulder].y + box_h * 0.11f));
  bones[kRightElbow] = ClampToBox(ImVec2(bones[kRightShoulder].x + box_w * 0.12f + arm_swing * 0.6f,
                                        bones[kRightShoulder].y + box_h * 0.11f));

  // Weapon hold positions (hands converge forward/right)
  const ImVec2 weapon_hand = ClampToBox(ImVec2(bones[kRightShoulder].x + hold_forward,
                                              bones[kRightShoulder].y + hold_down));
  bones[kRightHand] = ClampToBox(ImLerp(bones[kRightElbow], weapon_hand, 0.75f));
  bones[kLeftHand] = ClampToBox(ImLerp(bones[kLeftElbow], weapon_hand, 0.55f));

  // Legs with walk cycle and crouch compression
  const float leg_span = box_h * (0.40f + 0.08f * (1.0f - crouch_amt));
  const float knee_offset = box_h * 0.10f;
  bones[kLeftHip] = ClampToBox(ImVec2(bones[kHip].x - box_w * 0.08f, bones[kHip].y + box_h * 0.02f));
  bones[kRightHip] = ClampToBox(ImVec2(bones[kHip].x + box_w * 0.08f, bones[kHip].y + box_h * 0.02f));

  bones[kLeftKnee] = ClampToBox(ImVec2(bones[kLeftHip].x - box_w * 0.03f,
                                      bones[kLeftHip].y + knee_offset + leg_span * 0.32f + walk * box_h * 0.06f));
  bones[kRightKnee] = ClampToBox(ImVec2(bones[kRightHip].x + box_w * 0.03f,
                                       bones[kRightHip].y + knee_offset + leg_span * 0.32f + walk_opposite * box_h * 0.06f));

  bones[kLeftFoot] = ClampToBox(ImVec2(bones[kLeftKnee].x - box_w * 0.04f,
                                      bones[kLeftHip].y + leg_span + walk * box_h * 0.08f));
  bones[kRightFoot] = ClampToBox(ImVec2(bones[kRightKnee].x + box_w * 0.04f,
                                       bones[kRightHip].y + leg_span + walk_opposite * box_h * 0.08f));

  for (const auto &pair : connections) {
    draw->AddLine(bones[pair[0]], bones[pair[1]], col_secondary, 2.0f);
  }

  // Head circle (neck to spine3)
  const ImVec2 neck = bones[kNeck];
  const ImVec2 spine = bones[kSpine3];
  const float head_height = spine.y - neck.y;
  const ImVec2 head_pos = ImVec2(neck.x - (spine.x - neck.x) * 0.5f,
                                 neck.y - head_height * 0.5f);
  draw->AddCircle(ClampToBox(head_pos), head_height * 0.5f, col_secondary, 24, 2.0f);

  // Name tag
  const char *name = "Target";
  const ImVec2 text_size = ImGui::CalcTextSize(name);
  const ImU32 name_bg = ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.12f, 0.85f));
  draw->AddRectFilled(ImVec2(center.x - text_size.x * 0.5f - 6.0f, box_min.y - 26.0f),
                      ImVec2(center.x + text_size.x * 0.5f + 6.0f, box_min.y - 6.0f),
                      name_bg, 6.0f);
  draw->AddText(ImVec2(center.x - text_size.x * 0.5f, box_min.y - 24.0f),
                col_secondary, name);
}
