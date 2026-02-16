#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
//#include "examples/example_win32_directx9/imgui_settings.h"

void CustomStyleColor() // Отрисовка цветов
{
    ImGuiStyle& s = ImGui::GetStyle();
    ImGuiContext& g = *GImGui;

    s.Colors[ImGuiCol_Border] = ImColor(0, 0, 0, 0);
    s.Colors[ImGuiCol_PopupBg] = ImColor(7, 8, 18, 127);

    s.ChildRounding = 0.f;
    s.WindowRounding = 12.f;
    s.WindowPadding = ImVec2(0, 0);

    s.Colors[ImGuiCol_ChildBg] = ImColor(0, 0, 0, 255);
    s.Colors[ImGuiCol_WindowBg] = ImColor(7, 7, 7, 255);

}
ID3D11ShaderResourceView* bg = nullptr;
ImFont* Inter_S = nullptr;
ImFont* Inter_S_1 = nullptr;
ImFont* Inter_S_2 = nullptr;
ImFont* Inter_S_3 = nullptr;
ImFont* Inter_B = nullptr;
ImFont* Icon = nullptr;
ImFont* Icon_Arrow;

int sub_page = 0;
int page = 0;
static float tab_alpha = 0.f; /* */ static float tab_add; /* */ static int active_tab = 0;

int togle = 0;
static float anim_text = 0.f;
