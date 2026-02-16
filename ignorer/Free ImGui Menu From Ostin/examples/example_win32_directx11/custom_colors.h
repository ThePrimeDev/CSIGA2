#pragma once

#include "imgui.h"
class c_colors
{
public:


    ImVec4 accent = ImColor(235, 44, 35);
    ImVec4 accentglow = ImColor(255, 0, 0);
    ImVec4 white = ImColor(255, 255, 255);
    ImVec4 window_background = ImColor(3, 3, 3, 230);
    ImVec4 window_sidebar = ImColor(8, 8, 9, 240);
    ImVec4 window_stroke = ImColor(27, 27, 29);
    ImVec4 window_name = ImColor(255, 255, 255);

    ImVec4 scrollbar_line = ImColor(15, 15, 16);
    ImVec4 scrollbar_background = ImColor(20, 19, 24);
    ImVec4 scrollbar_stroke = ImColor(27, 27, 29);

    ImVec4 child_background = ImColor(12, 12, 13, 150);
    ImVec4 child_label = ImColor(201, 201, 203);

    ImVec4 selection_label_active = ImColor(255, 255, 255);
    ImVec4 selection_label_inactive = ImColor(135, 135, 136);
    ImVec4 selection_background = ImColor(24, 25, 29);


    ImVec4 checkbox_background = ImColor(42, 46, 50, 255);
    ImVec4 checkbox_stroke = ImColor(27, 27, 29);
    // ImVec4 checkbox_checkmark = ImColor(16, 16, 18);
    ImVec4 checkbox_checkmark = ImColor(255, 255, 255);
    // ImVec4 checkbox_label = ImColor(186, 186, 186);
    ImVec4 checkbox_label = ImColor(255, 255, 255);
    ImVec4 checkbox_label_in = ImColor(94, 94, 113, 255);
    ImVec4 checkbox_settings = ImColor(76, 76, 77);


    ImVec4 slider_background = ImColor(20, 24, 27);
    ImVec4 slider_stroke = ImColor(27, 27, 29);
    ImVec4 slider_label = ImColor(186, 186, 186);
    ImVec4 slider_circle = ImColor(255, 255, 255);

    ImVec4 dropdown_background = ImColor(20, 24, 27);
    ImVec4 dropdown_stroke = ImColor(27, 27, 29);
    ImVec4 dropdown_label = ImColor(186, 186, 186);
    ImVec4 dropdown_window = ImColor(3, 3, 3);

    ImVec4 selectable_label_inactive = ImColor(135, 135, 136);
    ImVec4 selectable_label_active = ImColor(255, 255, 255);

    ImVec4 text_field_background = ImColor(20, 24, 27);
    ImVec4 text_field_stroke = ImColor(27, 27, 29);
    ImVec4 text_field_label = ImColor(186, 186, 186);

    ImVec4 button_background = ImColor(20, 24, 27);
    ImVec4 button_stroke = ImColor(27, 27, 29);
    ImVec4 button_label_inactive = ImColor(186, 186, 186);
    ImVec4 button_label_active = ImColor(3, 3, 3);

    ImVec4 color_label = ImColor(186, 186, 186);
    ImVec4 color_background = ImColor(3, 3, 3);
    ImVec4 color_stroke = ImColor(27, 27, 29);
    ImVec4 color_cpbutton_background = ImColor(29, 30, 36);
    ImVec4 color_button_background = ImColor(16, 16, 18);
    ImVec4 color_button_label_inactive = ImColor(255, 255, 255);
    ImVec4 color_button_label_active = ImColor(3, 3, 3);

    ImVec4 key_background = ImColor(22, 22, 30, 255);
    ImVec4 key_background_wb = ImColor(18, 18, 24, 255);
    ImVec4 key_stroke = ImColor(27, 27, 29);
    ImVec4 key_button_background = ImColor(13, 13, 13);
    ImVec4 key_button_label = ImColor(255, 255, 255);
    ImVec4 key_red_button_label = ImColor(218, 112, 112);

    ImVec4 config_button_background = ImColor(27, 27, 29);
    ImVec4 config_button_label_inactive = ImColor(84, 84, 86);
    ImVec4 config_button_label_active = ImColor(16, 16, 18);
    ImVec4 config_label = ImColor(186, 186, 186);
    ImVec4 config_text = ImColor(101, 101, 102);

    ImVec4 notify_background = ImColor(16, 16, 18);
    ImVec4 notify_text = ImColor(125, 125, 125);

    ImVec4 listbox_background = ImColor(3, 3, 3);
    ImVec4 listbox_stroke = ImColor(27, 27, 29);

    ImVec4 watermark_background = ImColor(16, 16, 18);
    ImVec4 watermark_icon_rect = ImColor(27, 27, 29);
    ImVec4 watermark_text = ImColor(255, 255, 255);
    //ImVec4 watermark_small_text = ImColor(195, 195, 196);
    ImVec4 watermark_small_text = ImColor(255, 0, 0);
    ImVec4 watermark_line = ImColor(24, 24, 27);

};

inline c_colors* clr = new c_colors();
