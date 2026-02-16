#pragma once

#include <imgui.h>

namespace ui_widgets {

bool Checkbox(const char *label, bool *value);

bool Combo(const char *label,
           int *current_item,
           const char *const items[],
           int items_count,
           int height_in_items,
           ImFont *arrow_font);

bool SliderFloat(const char *label,
                 float *value,
                 float min_value,
                 float max_value,
                 const char *format);

bool InputText(const char *label, char *buf, size_t buf_size);

bool DotSlider(const char *label,
               float *value,
               float min_value,
               float max_value,
               const char *format);

bool KeyBind(const char *label, int *key, const char *empty_text);

bool ColorEdit4(const char *label, float col[4], ImGuiColorEditFlags flags);

}  // namespace ui_widgets
