#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include "imgui.h"
#include "imgui_internal.h"
#include "imstb_textedit.h"
#include "custom_colors.h"
#include "custom_settings.h"
#include <algorithm>

class c_widgets
{
public:
    template <typename T>
    T* anim_container(T** state_ptr, ImGuiID id)
    {
        T* state = static_cast<T*>(ImGui::GetStateStorage()->GetVoidPtr(id));
        if (!state) ImGui::GetStateStorage()->SetVoidPtr(id, state = new T());

        *state_ptr = state;
        return state;
    }

    ImU32 get_clr(const ImVec4& col, float alpha = 1.f);

    float fixed_speed(float speed) { return speed / ImGui::GetIO().Framerate; };

    bool color_edit(std::string_view label, float col[4], ImGuiColorEditFlags flags = 0);


};

inline c_widgets* gui = new c_widgets();

struct notify_struct {
    int id;
    std::string message, information;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    ImU32 color;
};

class c_notify {
public:
    c_notify() : notificationIdCounter(0) {}

    void AddNotification(const std::string& information, const std::string& message, int durationMs, ImU32 color) {
        auto now = std::chrono::steady_clock::now();
        auto endTime = now + std::chrono::milliseconds(durationMs);
        notifications.push_back({ notificationIdCounter++, information, message, now, endTime, color });
    }

    void DrawNotifications() {
        auto now = std::chrono::steady_clock::now();

        std::sort(notifications.rbegin(), notifications.rend(),
            [now](const notify_struct& a, const notify_struct& b) -> bool {
                float durationA = std::chrono::duration_cast<std::chrono::milliseconds>(a.endTime - a.startTime).count();
                float elapsedA = std::chrono::duration_cast<std::chrono::milliseconds>(now - a.startTime).count();
                float percentageA = (durationA - elapsedA) / durationA;

                float durationB = std::chrono::duration_cast<std::chrono::milliseconds>(b.endTime - b.startTime).count();
                float elapsedB = std::chrono::duration_cast<std::chrono::milliseconds>(now - b.startTime).count();
                float percentageB = (durationB - elapsedB) / durationB;

                return percentageA < percentageB;
            }
        );

        int currentNotificationPosition = 0;

        for (auto& notification : notifications) {
            if (now < notification.endTime) {
                float duration = std::chrono::duration_cast<std::chrono::milliseconds>(notification.endTime - notification.startTime).count();
                float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - notification.startTime).count();
                float percentage = (duration - elapsed) / duration * 100.0f;

                ShowNotification(currentNotificationPosition, notification.information, notification.message, percentage, notification.color);
                currentNotificationPosition++;
            }
        }

        notifications.erase(std::remove_if(notifications.begin(), notifications.end(),
            [now](const notify_struct& notification) { return now >= notification.endTime; }),
            notifications.end());
    }

private:
    std::vector<notify_struct> notifications;
    int notificationIdCounter;

    void ShowNotification(int position, const std::string& information, const std::string& message, float percentage, ImU32 color) {

        //ImGui::SetNextWindowPos(ImVec2(1570, 950 + (position * 76)), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(1570, 950 - (position * 76)), ImGuiCond_Always);


        ImGui::SetNextWindowSize(ImVec2(335, 60));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);

        ImGui::Begin(("##NOTIFY" + std::to_string(position)).c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);
        {
            const ImVec2 pos = ImGui::GetWindowPos();
            const ImVec2 size = ImGui::GetWindowSize();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImGuiStyle* style = &ImGui::GetStyle();

            { // style
                style->WindowPadding = set->window_padding;
                style->WindowBorderSize = set->window_border_size;
                style->WindowRounding = set->widgets_rounding;
            }

            { // decorations

                //ImGui::GetForegroundDrawList()->AddShadowRect(pos, pos + ImVec2(size.x, 0), color, 50.f, ImVec2(0, 0), 5.f, 4.f);

                draw->AddRectFilled(pos, pos + ImVec2(size.x, 10), color, 4.f);




                draw->AddRectFilled(pos + ImVec2(0, 2), pos + size, gui->get_clr(clr->notify_background), 4.f);

                draw->AddCircleFilled(pos + ImVec2(30, 30), 6.f, color, 30);

                //    ImGui::GetForegroundDrawList()->AddShadowCircle(pos + ImVec2(30, 30), 6.f, color, 50.f, ImVec2(0, 0), 0, 12);

                draw->PathArcTo(pos + ImVec2(30, 30), 13.f, 0, IM_PI * 2, 60);
                draw->PathStroke(gui->get_clr(ImVec4(0.f, 0.f, 0.f, 0.5f)), 0, 3);

                draw->PathArcTo(pos + ImVec2(30, 30), 13.f, 0, (IM_PI * 2) * percentage / 100, 60);
                draw->PathStroke(color, 0, 3);

                draw->AddText(set->value_font, set->value_font->FontSize, pos + ImVec2(65, 14), color, information.c_str());
                draw->AddText(set->value_font, set->value_font->FontSize, pos + ImVec2(65, 31), gui->get_clr(clr->notify_text), message.c_str());
            }
        }
        ImGui::End();

        ImGui::PopStyleVar(2);
    }
};

inline c_notify* notify = new c_notify();
