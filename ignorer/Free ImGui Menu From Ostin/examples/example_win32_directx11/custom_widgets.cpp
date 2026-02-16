#include "custom_widgets.h"
#include "imstb_textedit.h"
#include <wtypes.h>

using namespace ImGui;
using namespace ImStb;


ImU32 c_widgets::get_clr(const ImVec4& col, float alpha)
{
    ImGuiStyle& style = GImGui->Style;
    ImVec4 c = col;
    c.w *= style.Alpha * alpha;
    return ColorConvertFloat4ToU32(c);
}

void add_rect_filled_multi_color(ImDrawList* draw, const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left, float rounding = 0.f, ImDrawFlags flags = 0)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    auto fix_rect_corner_flags = [](ImDrawFlags rflags)
        {
            if ((rflags & ImDrawFlags_RoundCornersMask_) == 0)
                rflags |= ImDrawFlags_RoundCornersAll;
            return rflags;
        };

    flags = fix_rect_corner_flags(flags);
    rounding = ImMin(rounding, ImFabs(p_max.x - p_min.x) * (((flags & ImDrawFlags_RoundCornersTop) == ImDrawFlags_RoundCornersTop) || ((flags & ImDrawFlags_RoundCornersBottom) == ImDrawFlags_RoundCornersBottom) ? 0.5f : 1.0f) - 1.0f);
    rounding = ImMin(rounding, ImFabs(p_max.y - p_min.y) * (((flags & ImDrawFlags_RoundCornersLeft) == ImDrawFlags_RoundCornersLeft) || ((flags & ImDrawFlags_RoundCornersRight) == ImDrawFlags_RoundCornersRight) ? 0.5f : 1.0f) - 1.0f);

    if (rounding > 0.0f)
    {
        const int size_before = draw->VtxBuffer.Size;
        draw->AddRectFilled(p_min, p_max, IM_COL32_WHITE, rounding, flags);
        const int size_after = draw->VtxBuffer.Size;

        for (int i = size_before; i < size_after; i++)
        {
            ImDrawVert* vert = draw->VtxBuffer.Data + i;

            ImVec4 upr_left = ImGui::ColorConvertU32ToFloat4(col_upr_left);
            ImVec4 bot_left = ImGui::ColorConvertU32ToFloat4(col_bot_left);
            ImVec4 up_right = ImGui::ColorConvertU32ToFloat4(col_upr_right);
            ImVec4 bot_right = ImGui::ColorConvertU32ToFloat4(col_bot_right);

            float X = ImClamp((vert->pos.x - p_min.x) / (p_max.x - p_min.x), 0.0f, 1.0f);

            // 4 colors - 8 deltas

            float r1 = upr_left.x + (up_right.x - upr_left.x) * X;
            float r2 = bot_left.x + (bot_right.x - bot_left.x) * X;

            float g1 = upr_left.y + (up_right.y - upr_left.y) * X;
            float g2 = bot_left.y + (bot_right.y - bot_left.y) * X;

            float b1 = upr_left.z + (up_right.z - upr_left.z) * X;
            float b2 = bot_left.z + (bot_right.z - bot_left.z) * X;

            float a1 = upr_left.w + (up_right.w - upr_left.w) * X;
            float a2 = bot_left.w + (bot_right.w - bot_left.w) * X;


            float Y = ImClamp((vert->pos.y - p_min.y) / (p_max.y - p_min.y), 0.0f, 1.0f);
            float r = r1 + (r2 - r1) * Y;
            float g = g1 + (g2 - g1) * Y;
            float b = b1 + (b2 - b1) * Y;
            float a = a1 + (a2 - a1) * Y;
            ImVec4 RGBA(r, g, b, a);

            RGBA = RGBA * ImGui::ColorConvertU32ToFloat4(vert->col);

            vert->col = ImColor(RGBA);
        }
        return;
    }

    const ImVec2 uv = draw->_Data->TexUvWhitePixel;
    draw->PrimReserve(6, 4);
    draw->PrimWriteIdx((ImDrawIdx)(draw->_VtxCurrentIdx)); draw->PrimWriteIdx((ImDrawIdx)(draw->_VtxCurrentIdx + 1)); draw->PrimWriteIdx((ImDrawIdx)(draw->_VtxCurrentIdx + 2));
    draw->PrimWriteIdx((ImDrawIdx)(draw->_VtxCurrentIdx)); draw->PrimWriteIdx((ImDrawIdx)(draw->_VtxCurrentIdx + 2)); draw->PrimWriteIdx((ImDrawIdx)(draw->_VtxCurrentIdx + 3));
    draw->PrimWriteVtx(p_min, uv, col_upr_left);
    draw->PrimWriteVtx(ImVec2(p_max.x, p_min.y), uv, col_upr_right);
    draw->PrimWriteVtx(p_max, uv, col_bot_right);
    draw->PrimWriteVtx(ImVec2(p_min.x, p_max.y), uv, col_bot_left);
}

std::string imvec4_to_hex(const ImVec4& color)
{
    auto to_hex = [](int value)
        {
            std::stringstream stream;
            if (value < 16)
                stream << "0";

            stream << std::hex << std::uppercase << value;

            return stream.str();
        };

    int r = static_cast<int>(color.x * 255.0f);
    int g = static_cast<int>(color.y * 255.0f);
    int b = static_cast<int>(color.z * 255.0f);

    std::string hex_color = "#" + to_hex(r) + to_hex(g) + to_hex(b);

    return hex_color;
}



bool color_button(const char* desc_id, const ImVec4& col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(desc_id);
    const float default_size = GetFrameHeight();
    const ImVec2 pos = window->DC.CursorPos;
    const float width = GetWindowWidth() - g.Style.WindowPadding.x * 2;
    const ImRect rect(pos, pos + ImVec2(width, 18));
    ItemSize(rect, 0.0f);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);

    RenderColorRectWithAlphaCheckerboard(window->DrawList, ImVec2(rect.Max.x - 18, rect.Min.y), rect.Max, gui->get_clr(col), 9.f, ImVec2(0, 0), set->widgets_rounding);
    window->DrawList->AddText(set->widgets_font, set->widgets_font->FontSize, rect.Min - ImVec2(1, -1), gui->get_clr(clr->color_label), desc_id);

    window->DrawList->AddText(set->value_font, set->value_font->FontSize, ImVec2(rect.Max.x - 28 - set->value_font->CalcTextSizeA(set->value_font->FontSize, FLT_MAX, -1.f, imvec4_to_hex(col).c_str()).x, rect.Min.y + 2), gui->get_clr(clr->color_label), imvec4_to_hex(col).c_str());

    return pressed;
}

static void color_edit_restore_h(const float* col, float* H)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
        return;
    *H = g.ColorEditSavedHue;
}

static void color_edit_restore_hs(const float* col, float* H, float* S, float* V)
{
    ImGuiContext& g = *GImGui;
    IM_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
        return;

    // When S == 0, H is undefined.
    // When H == 1 it wraps around to 0.
    if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
        *H = g.ColorEditSavedHue;

    // When V == 0, S is undefined.
    if (*V == 0.0f)
        *S = g.ColorEditSavedSat;
}

bool color_picker(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImDrawList* draw_list = window->DrawList;
    ImGuiStyle& style = g.Style;
    ImGuiIO& io = g.IO;

    const float width = CalcItemWidth();
    const bool is_readonly = ((g.NextItemData.ItemFlags | g.CurrentItemFlags) & ImGuiItemFlags_ReadOnly) != 0;
    g.NextItemData.ClearFlags();

    PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id)
        g.ColorEditCurrentID = window->IDStack.back();
    BeginGroup();

    if (!(flags & ImGuiColorEditFlags_NoSidePreview))
        flags |= ImGuiColorEditFlags_NoSmallPreview;

    // Context menu: display and store options.
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        ColorPickerOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & ImGuiColorEditFlags_PickerMask_))
        flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_PickerMask_;
    if (!(flags & ImGuiColorEditFlags_InputMask_))
        flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_InputMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_InputMask_;
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_PickerMask_)); // Check that only 1 is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));  // Check that only 1 is selected
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

    // Setup
    int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
    bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
    ImVec2 picker_pos = window->DC.CursorPos;
    ImVec2 picker_size = ImVec2(151, 151);
    ImVec2 bars_size = ImVec2(10, picker_size.y);
    ImVec2 hue_bar_pos = picker_pos + ImVec2(g.Style.WindowPadding.x + picker_size.x, 0);
    ImVec2 alpha_bar_pos = picker_pos + ImVec2(g.Style.WindowPadding.x * 2 + picker_size.x + bars_size.x, 0);

    float backup_initial_col[4];
    memcpy(backup_initial_col, col, components * sizeof(float));

    float H = col[0], S = col[1], V = col[2];
    float R = col[0], G = col[1], B = col[2];
    if (flags & ImGuiColorEditFlags_InputRGB)
    {
        // Hue is lost when converting from grayscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(R, G, B, H, S, V);
        color_edit_restore_hs(col, &H, &S, &V);
    }
    else if (flags & ImGuiColorEditFlags_InputHSV)
    {
        ColorConvertHSVtoRGB(H, S, V, R, G, B);
    }

    bool value_changed = false, value_changed_h = false, value_changed_sv = false;

    PushItemFlag(ImGuiItemFlags_NoNav, true);
    // SV rectangle logic
    InvisibleButton("sv", picker_size);
    if (IsItemActive() && !is_readonly)
    {
        S = ImSaturate((io.MousePos.x - picker_pos.x) / (picker_size.x - 1));
        V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (picker_size.y - 1));
        color_edit_restore_h(col, &H); // Greatly reduces hue jitter and reset to 0 when hue == 255 and color is rapidly modified using SV square.
        value_changed = value_changed_sv = true;
    }

    // Hue bar logic
    SetCursorScreenPos(hue_bar_pos);
    InvisibleButton("hue", bars_size);
    if (IsItemActive() && !is_readonly)
    {
        H = ImSaturate((io.MousePos.y - picker_pos.y) / (bars_size.y - 1));
        value_changed = value_changed_h = true;
    }

    // Alpha bar logic
    if (alpha_bar)
    {
        SetCursorScreenPos(alpha_bar_pos);
        InvisibleButton("alpha", bars_size);
        if (IsItemActive())
        {
            col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (bars_size.y - 1));
            value_changed = true;
        }
    }
    PopItemFlag(); // ImGuiItemFlags_NoNav

    // Convert back color to RGB
    if (value_changed_h || value_changed_sv)
    {
        if (flags & ImGuiColorEditFlags_InputRGB)
        {
            ColorConvertHSVtoRGB(H, S, V, col[0], col[1], col[2]);
            g.ColorEditSavedHue = H;
            g.ColorEditSavedSat = S;
            g.ColorEditSavedID = g.ColorEditCurrentID;
            g.ColorEditSavedColor = ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0));
        }
        else if (flags & ImGuiColorEditFlags_InputHSV)
        {
            col[0] = H;
            col[1] = S;
            col[2] = V;
        }
    }

    // R,G,B and H,S,V slider color editor
    bool value_changed_fix_hue_wrap = false;

    // Try to cancel hue wrap (after ColorEdit4 call), if any
    if (value_changed_fix_hue_wrap && (flags & ImGuiColorEditFlags_InputRGB))
    {
        float new_H, new_S, new_V;
        ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
        if (new_H <= 0 && H > 0)
        {
            if (new_V <= 0 && V != new_V)
                ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
            else if (new_S <= 0)
                ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
        }
    }

    if (value_changed)
    {
        if (flags & ImGuiColorEditFlags_InputRGB)
        {
            R = col[0];
            G = col[1];
            B = col[2];
            ColorConvertRGBtoHSV(R, G, B, H, S, V);
            color_edit_restore_hs(col, &H, &S, &V);   // Fix local Hue as display below will use it immediately.
        }
        else if (flags & ImGuiColorEditFlags_InputHSV)
        {
            H = col[0];
            S = col[1];
            V = col[2];
            ColorConvertHSVtoRGB(H, S, V, R, G, B);
        }
    }

    const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
    const ImU32 col_black = IM_COL32(0, 0, 0, style_alpha8);
    const ImU32 col_white = IM_COL32(255, 255, 255, style_alpha8);
    const ImU32 col_midgrey = IM_COL32(128, 128, 128, style_alpha8);
    const ImU32 col_hues[6 + 1] = { IM_COL32(255,0,0,style_alpha8), IM_COL32(255,255,0,style_alpha8), IM_COL32(0,255,0,style_alpha8), IM_COL32(0,255,255,style_alpha8), IM_COL32(0,0,255,style_alpha8), IM_COL32(255,0,255,style_alpha8), IM_COL32(255,0,0,style_alpha8) };

    ImVec4 hue_color_f(1, 1, 1, style.Alpha); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
    ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
    ImU32 user_col32_striped_of_alpha = ColorConvertFloat4ToU32(ImVec4(R, G, B, style.Alpha)); // Important: this is still including the main rendering/style alpha!!

    ImVec2 sv_cursor_pos;

    // Render SV Square
    add_rect_filled_multi_color(draw_list, picker_pos, picker_pos + picker_size, col_white, hue_color32, hue_color32, col_white, set->widgets_rounding);
    add_rect_filled_multi_color(draw_list, picker_pos, picker_pos + picker_size, 0, 0, col_black, col_black);

    sv_cursor_pos.x = ImClamp(IM_ROUND(picker_pos.x + ImSaturate(S) * picker_size.x), picker_pos.x + 2, picker_pos.x + picker_size.x - 2); // Sneakily prevent the circle to stick out too much
    sv_cursor_pos.y = ImClamp(IM_ROUND(picker_pos.y + ImSaturate(1 - V) * picker_size.y), picker_pos.y + 2, picker_pos.y + picker_size.y - 2);

    // Render Hue Bar
    for (int i = 0; i < 6; ++i)
        add_rect_filled_multi_color(draw_list, hue_bar_pos + ImVec2(0, i * (bars_size.y / 6)), hue_bar_pos + ImVec2(bars_size.x, (i + 1) * (bars_size.y / 6)), col_hues[i], col_hues[i], col_hues[i + 1], col_hues[i + 1], set->widgets_rounding, i == 0 ? ImDrawFlags_RoundCornersTop : i == 5 ? ImDrawFlags_RoundCornersBottom : ImDrawFlags_RoundCornersNone);

    float hue_bar_circle_pos = IM_ROUND(H * bars_size.y);
    hue_bar_circle_pos = ImClamp(hue_bar_circle_pos, 5.f, bars_size.y - 5.f);
    // draw_list->AddShadowCircle(hue_bar_pos + ImVec2(bars_size.x / 2, hue_bar_circle_pos), bars_size.x / 2, col_black, 30.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
    draw_list->AddCircle(hue_bar_pos + ImVec2(bars_size.x / 2, hue_bar_circle_pos), bars_size.x / 2, col_white, 30);

    // Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
    //draw_list->AddShadowCircle(sv_cursor_pos, bars_size.x / 2, col_black, 30.f, ImVec2(0, 0));
    draw_list->AddCircleFilled(sv_cursor_pos, bars_size.x / 2, user_col32_striped_of_alpha, 20);
    draw_list->AddCircle(sv_cursor_pos, bars_size.x / 2, col_white, 20);

    // Render alpha bar
    if (alpha_bar)
    {
        float alpha = ImSaturate(col[3]);
        add_rect_filled_multi_color(draw_list, alpha_bar_pos, alpha_bar_pos + bars_size, user_col32_striped_of_alpha, user_col32_striped_of_alpha, user_col32_striped_of_alpha & ~IM_COL32_A_MASK, user_col32_striped_of_alpha & ~IM_COL32_A_MASK, set->widgets_rounding);
        float alpha_bar_circle_pos = IM_ROUND((1.0f - alpha) * bars_size.y);
        alpha_bar_circle_pos = ImClamp(alpha_bar_circle_pos, 5.f, bars_size.y - 5.f);
        // draw_list->AddShadowCircle(alpha_bar_pos + ImVec2(bars_size.x / 2, alpha_bar_circle_pos), bars_size.x / 2, col_black, 30.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
        draw_list->AddCircle(alpha_bar_pos + ImVec2(bars_size.x / 2, alpha_bar_circle_pos), bars_size.x / 2, col_white, 30);

    }

    EndGroup();

    if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
        value_changed = false;
    if (value_changed && g.LastItemData.ID != 0) // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        MarkItemEdited(g.LastItemData.ID);

    if (set_current_color_edit_id)
        g.ColorEditCurrentID = 0;
    PopID();

    return value_changed;
}

bool copy_and_paste_button(std::string_view label)
{
    struct clrbutton_state
    {
        float alpha = 0.f;
        ImVec4 label_clr = clr->color_button_label_inactive;
        bool clicked = false;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, pos + ImVec2(88, 25));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);

    // Render
    clrbutton_state* state = gui->anim_container(&state, id);

    if (pressed)
        state->clicked = true;

    state->alpha = ImClamp(state->alpha + (gui->fixed_speed(6.f) * (state->clicked ? 1.f : -1.f)), 0.f, 1.f);
    state->label_clr = ImLerp(state->label_clr, state->clicked ? clr->color_button_label_active : clr->color_button_label_inactive, gui->fixed_speed(20.f));

    if (state->alpha >= 0.99f)
        state->clicked = false;

    window->DrawList->AddRectFilled(rect.Min, rect.Max, gui->get_clr(clr->color_cpbutton_background), set->widgets_rounding);
    window->DrawList->AddRectFilled(rect.Min, rect.Max, gui->get_clr(clr->accent, state->alpha), set->widgets_rounding);

    // gui->push_font(set->value_font);
    // gui->push_style_color(ImGuiCol_Text, gui->get_clr(state->label_clr));
    // gui->render_text_clipped(rect.Min - ImVec2(0, 1), rect.Max - ImVec2(0, 1), label.data(), NULL, NULL, ImVec2(0.5f, 0.5f));
    // gui->pop_style_color();
    // gui->pop_font();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool pipette_button(std::string_view label, bool* callback)
{
    struct pipette_state
    {
        ImVec4 icon_clr = clr->button_label_inactive;
        float alpha = 0.f;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + ImVec2(25, 25));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
    {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
        return false;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
    {
        *callback = !(*callback);
        MarkItemEdited(id);
    }

    pipette_state* state = gui->anim_container(&state, id);

    state->alpha = ImClamp(state->alpha + (gui->fixed_speed(6.f) * (*callback ? 1.f : -1.f)), 0.f, 1.f);
    state->icon_clr = ImLerp(state->icon_clr, *callback ? clr->color_button_label_active : clr->color_button_label_inactive, gui->fixed_speed(20.f));

    window->DrawList->AddRectFilled(rect.Min, rect.Max, gui->get_clr(clr->color_button_background), set->widgets_rounding);
    window->DrawList->AddRectFilled(rect.Min, rect.Max, gui->get_clr(clr->accent, state->alpha), set->widgets_rounding);
    window->DrawList->AddText(set->colorpicker_icon_font, set->colorpicker_icon_font->FontSize, rect.Min + ImVec2(7, 7), gui->get_clr(state->icon_clr), label.data());

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*callback ? ImGuiItemStatusFlags_Checked : 0));
    return pressed;
}

bool plus_color_button(std::string_view label)
{
    struct plus_color_state
    {
        ImVec4 icon_clr = clr->button_label_inactive;
        float alpha = 0.f;
        bool clicked = false;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());
    ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, pos + ImVec2(25, 25));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);

    // Render

    plus_color_state* state = gui->anim_container(&state, id);

    if (pressed)
        state->clicked = true;

    state->alpha = ImClamp(state->alpha + (gui->fixed_speed(6.f) * (state->clicked ? 1.f : -1.f)), 0.f, 1.f);
    state->icon_clr = ImLerp(state->icon_clr, state->clicked ? clr->button_label_active : clr->button_label_inactive, gui->fixed_speed(20.f));

    if (state->alpha >= 0.99f)
        state->clicked = false;

    window->DrawList->AddRectFilled(rect.Min, rect.Max, gui->get_clr(clr->color_button_background), set->widgets_rounding);
    window->DrawList->AddRectFilled(rect.Min, rect.Max, gui->get_clr(clr->accent, state->alpha), set->widgets_rounding);
    window->DrawList->AddText(set->colorpicker_icon_font, set->colorpicker_icon_font->FontSize, rect.Min + ImVec2(7, 7), gui->get_clr(state->icon_clr), label.data());

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool stored_color_button(std::string_view label, ImVec4& col)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());
    ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, pos + ImVec2(17, 17));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);

    // Render
    float* state = gui->anim_container(&state, id);

    *state = ImClamp(*state + (gui->fixed_speed(6.f) * (hovered ? 1.f : -1.f)), 0.f, 1.f);

    RenderColorRectWithAlphaCheckerboard(window->DrawList, rect.Min, rect.Max, gui->get_clr(col), rect.GetWidth() / 2, ImVec2(0, 0), 3.f);
    window->DrawList->AddRect(rect.Min - ImVec2(1, 1), rect.Max + ImVec2(1, 1), gui->get_clr(ImVec4(1.f, 1.f, 1.f, *state)), 4.f);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool c_widgets::color_edit(std::string_view label, float col[4], ImGuiColorEditFlags flags)
{
    struct color_state
    {
        bool active = false;
        bool hovered = false;
        bool pipette_active = false;
        float additional_height = 0.f;
        float alpha = 0.f;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float square_sz = GetFrameHeight();
    const char* label_display_end = FindRenderedTextEnd(label.data());
    float w_full = CalcItemWidth();
    g.NextItemData.ClearFlags();

    BeginGroup();
    PushID(label.data());
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id)
        g.ColorEditCurrentID = window->IDStack.back();

    // If we're not showing any slider there's no point in doing any HSV conversions
    const ImGuiColorEditFlags flags_untouched = flags;
    if (flags & ImGuiColorEditFlags_NoInputs)
        flags = (flags & (~ImGuiColorEditFlags_DisplayMask_)) | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoOptions;

    // Context menu: display and modify options (before defaults are applied)
    if (!(flags & ImGuiColorEditFlags_NoOptions))
        ColorEditOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & ImGuiColorEditFlags_DisplayMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DisplayMask_);
    if (!(flags & ImGuiColorEditFlags_DataTypeMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DataTypeMask_);
    if (!(flags & ImGuiColorEditFlags_PickerMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_);
    if (!(flags & ImGuiColorEditFlags_InputMask_))
        flags |= (g.ColorEditOptions & ImGuiColorEditFlags_InputMask_);
    flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_));
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_DisplayMask_)); // Check that only 1 is selected
    IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));   // Check that only 1 is selected

    const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
    const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
    const int components = alpha ? 4 : 3;
    const float w_button = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
    const float w_inputs = ImMax(w_full - w_button, 1.0f);
    w_full = w_inputs + w_button;

    // Convert to the formats we need
    float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
    if ((flags & ImGuiColorEditFlags_InputHSV) && (flags & ImGuiColorEditFlags_DisplayRGB))
        ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
    else if ((flags & ImGuiColorEditFlags_InputRGB) && (flags & ImGuiColorEditFlags_DisplayHSV))
    {
        // Hue is lost when converting from grayscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
        color_edit_restore_hs(col, &f[0], &f[1], &f[2]);
    }
    int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

    bool value_changed = false;
    bool value_changed_as_float = false;

    const ImVec2 pos = window->DC.CursorPos;
    const float inputs_offset_x = (style.ColorButtonPosition == ImGuiDir_Left) ? w_button : 0.0f;

    color_state* state = gui->anim_container(&state, GetID(label.data()));

    ImGuiWindow* picker_active_window = NULL;
    if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
    {
        const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
        if (color_button(label.data(), col_v4))
        {
            if (!(flags & ImGuiColorEditFlags_NoPicker))
            {
                // Store current color and open a picker
                g.ColorPickerRef = col_v4;
            }
        }

        if (ItemHoverable(g.LastItemData.Rect, g.LastItemData.ID) && g.IO.MouseClicked[0] || (state->active && g.IO.MouseClicked[0] && !state->hovered) && !state->pipette_active)
            state->active = !state->active;

        state->alpha = ImClamp(state->alpha + (8.f * g.IO.DeltaTime * (state->active ? 1.f : -1.f)), 0.f, 1.f);

        if (!IsRectVisible(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max + ImVec2(0, 2)))
        {
            state->active = false;
            state->alpha = 0.f;
        }

        SetNextWindowSize(ImVec2(191, 221 + IM_ROUND(state->additional_height)));
        SetNextWindowPos(g.LastItemData.Rect.GetBR() - ImVec2(18, -5));
        PushStyleVar(ImGuiStyleVar_Alpha, state->alpha);
        PushStyleVar(ImGuiStyleVar_WindowRounding, set->widgets_rounding);
        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        PushStyleVar(ImGuiStyleVar_ItemSpacing, GetStyle().WindowPadding);
        PushStyleColor(ImGuiCol_WindowBg, gui->get_clr(clr->color_background));
        PushStyleColor(ImGuiCol_Border, gui->get_clr(clr->color_stroke));
        if (state->alpha >= 0.01f);
        {
            ImGui::Begin("picker_window", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
            {
                state->hovered = IsWindowHovered();
                picker_active_window = g.CurrentWindow;

                if (copy_and_paste_button("COPY"))
                    set->colorpicker_temp_buf = ImVec4(col[0], col[1], col[2], col[3]);

                //gui->sameline();

                if (copy_and_paste_button("PASTE"))
                    if (set->colorpicker_temp_buf.x != -1.f)
                    {
                        col[0] = set->colorpicker_temp_buf.x;
                        col[1] = set->colorpicker_temp_buf.y;
                        col[2] = set->colorpicker_temp_buf.z;
                        col[3] = flags & ImGuiColorEditFlags_AlphaBar ? set->colorpicker_temp_buf.w : 1.f;
                    }

                ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
                ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
                value_changed |= color_picker("##picker", col, picker_flags, &g.ColorPickerRef.x);

                pipette_button("E", &state->pipette_active);
                if (state->pipette_active && g.IO.MouseClicked[0] && !state->hovered)
                {
                    HDC hdc_screen = GetDC(NULL);
                    COLORREF pick_color = GetPixel(hdc_screen, GetMousePos().x, GetMousePos().y);
                    ReleaseDC(NULL, hdc_screen);

                    col[0] = static_cast<float>(GetRValue(pick_color)) / 255.0f;
                    col[1] = static_cast<float>(GetGValue(pick_color)) / 255.0f;
                    col[2] = static_cast<float>(GetBValue(pick_color)) / 255.0f;

                    state->pipette_active = false;
                }

                picker_active_window->DrawList->AddRectFilled(picker_active_window->DC.CursorPos + ImVec2(30, -30), picker_active_window->DC.CursorPos + ImVec2(151, -5), gui->get_clr(clr->color_button_background), set->widgets_rounding);
                //gui->push_font(set->value_font);
                //gui->push_style_color(ImGuiCol_Text, gui->get_clr(clr->color_button_label_inactive));
                //gui->render_text_clipped(picker_active_window->DC.CursorPos + ImVec2(30, -31), picker_active_window->DC.CursorPos + ImVec2(151, -6), imvec4_to_hex(ImVec4(col[0], col[1], col[2], col[3])).c_str(), NULL, NULL, ImVec2(0.5f, 0.5f));
                //gui->pop_style_color();
                //gui->pop_font();

                SetCursorPos(ImVec2(161, 191));
                if (plus_color_button("F") && set->colorpicker_stored_colors.size() < 14)
                {
                    set->colorpicker_stored_colors.insert(set->colorpicker_stored_colors.begin(), ImVec4(col[0], col[1], col[2], col[3]));
                }

                PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 5));
                SetCursorPosX(GetCursorPosX() + 1);
                BeginGroup();
                {
                    for (int i = 0; i < set->colorpicker_stored_colors.size(); i++)
                    {
                        if (stored_color_button((std::stringstream{} << "color" << i).str().c_str(), set->colorpicker_stored_colors.at(i)))
                        {
                            col[0] = set->colorpicker_stored_colors.at(i).x;
                            col[1] = set->colorpicker_stored_colors.at(i).y;
                            col[2] = set->colorpicker_stored_colors.at(i).z;
                            col[3] = flags & ImGuiColorEditFlags_AlphaBar ? set->colorpicker_stored_colors.at(i).w : 1.f;
                        }

                        if (IsItemClicked(ImGuiMouseButton_Right))
                            set->colorpicker_stored_colors.erase(set->colorpicker_stored_colors.begin() + i);

                        //  if (i != 6)
                         //     gui->sameline();
                    }
                }
                EndGroup();
                PopStyleVar();

                state->additional_height = ImLerp(state->additional_height, set->colorpicker_stored_colors.size() > 0 && set->colorpicker_stored_colors.size() < 8 ? 22.f : set->colorpicker_stored_colors.size() >= 8 ? 44.f : 0.f, gui->fixed_speed(10.f));
            }
            ImGui::End();
        }
        PopStyleVar(5);
        PopStyleColor(2);
    }

    // Convert back
    if (value_changed && picker_active_window == NULL)
    {
        if (!value_changed_as_float)
            for (int n = 0; n < 4; n++)
                f[n] = i[n] / 255.0f;
        if ((flags & ImGuiColorEditFlags_DisplayHSV) && (flags & ImGuiColorEditFlags_InputRGB))
        {
            g.ColorEditSavedHue = f[0];
            g.ColorEditSavedSat = f[1];
            ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
            g.ColorEditSavedID = g.ColorEditCurrentID;
            g.ColorEditSavedColor = ColorConvertFloat4ToU32(ImVec4(f[0], f[1], f[2], 0));
        }
        if ((flags & ImGuiColorEditFlags_DisplayRGB) && (flags & ImGuiColorEditFlags_InputHSV))
            ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);

        col[0] = f[0];
        col[1] = f[1];
        col[2] = f[2];
        if (alpha)
            col[3] = f[3];
    }

    if (set_current_color_edit_id)
        g.ColorEditCurrentID = 0;
    PopID();
    EndGroup();

    // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
    if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
        g.LastItemData.ID = g.ActiveId;

    if (value_changed && g.LastItemData.ID != 0) // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}
