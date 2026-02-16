#pragma once

#include <imgui.h>

struct UiAssets {
  ImTextureID background = 0;
  int background_width = 0;
  int background_height = 0;
  ImTextureID icon_user = 0;
  ImTextureID icon_fps = 0;
  ImTextureID icon_ms = 0;
  ImTextureID icon_logo = 0;
  ImFont *inter_s = nullptr;
  ImFont *inter_s1 = nullptr;
  ImFont *inter_s2 = nullptr;
  ImFont *inter_s3 = nullptr;
  ImFont *inter_b = nullptr;
  ImFont *icon = nullptr;
  ImFont *icon_arrow = nullptr;
};

UiAssets &GetUiAssets();
void LoadUiAssets();
void UnloadUiAssets();
