#pragma once
#include "imgui.h"

namespace c {

    inline ImVec4 main_color = ImColor(121, 106, 231, 255);
    inline ImVec4 text_active = ImColor(255, 255, 255, 255);
    inline ImVec4 text_in_active = ImColor(255, 255, 255, 127);
    inline ImVec4 child_rect = ImColor(0, 0, 0, 140);

    inline ImVec4 tab_active = ImColor(11, 11, 11, 255);

    inline ImVec4 tab_in_active = ImColor(0, 0, 0, 255);
    inline ImVec4 checkbox_bg_in_active = ImColor(9, 9, 9, 255);
    inline ImVec4 black = ImColor(0, 0, 0, 255);

    inline ImVec4 input = ImColor(11,11,11, 255);
    inline ImVec4 key_background_wb = ImColor(0, 0, 0, 255);

    inline ImVec4 select_tab_active = ImColor(129, 99, 255, 255);

    inline ImVec4 select_tab_inactive = ImColor(129, 99, 255, 0);

    inline ImVec4 settings_checkbox = ImColor(37, 37, 37, 255);

    inline ImVec4 circle_active = ImColor(121, 106, 231, 255);

    inline ImVec4 circle_inactive = ImColor(121, 106, 231, 127);

    inline ImVec4 circle_selected = ImColor(255, 255, 255, 255);

    inline ImVec4 circle_in_selected = ImColor(255, 255, 255, 0);

    inline ImVec4 background_slider = ImColor(9, 9, 9, 255);

    inline ImVec4 window = ImColor(7, 7,7, 255);

    namespace elements
    {
        inline ImVec4 mark = ImColor(255, 255, 255);

        inline ImVec4 stroke = ImColor(28, 26, 37);
        inline ImVec4 background = ImColor(15, 15, 17);
        inline ImVec4 background_widget = ImColor(21, 23, 26);

        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hov = ImColor(81, 92, 109);
        inline ImVec4 text = ImColor(43, 51, 63);

        inline float rounding = 4;
    }
    namespace background
    {

        inline ImVec4 filling = ImColor(12, 12, 12);
        inline ImVec4 stroke = ImColor(24, 26, 36);
        inline ImVec2 size = ImVec2(900, 515);

        inline float rounding = 6;

    }
}



