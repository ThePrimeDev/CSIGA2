// Dear ImGui: standalone example application for SDL3 + OpenGL3
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "main.h"
#include "Font.h"
#include "image.h"
#include "imgui_settings.h"
#include "custom_widgets.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "../../../../src/ui/stb_image.h"

static int notify_select = 0;
const char* notify_items[2]{ "Circle", "Line" };

namespace image
{
    unsigned int settings_checkbox = 0;
    unsigned int user = 0;
    unsigned int fps = 0;
    unsigned int ms = 0;
    unsigned int logo = 0;
}

static ImTextureID ToImTextureID(unsigned int texture)
{
    return (ImTextureID)(intptr_t)texture;
}

static void DestroyTexture(unsigned int* texture)
{
    if (!texture || *texture == 0)
    {
        return;
    }

    GLuint gl_texture = static_cast<GLuint>(*texture);
    glDeleteTextures(1, &gl_texture);
    *texture = 0;
}

static bool LoadTextureFromMemory(const unsigned char* data, size_t data_size, unsigned int* out_texture)
{
    if (!data || data_size == 0 || !out_texture)
    {
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(1);
    unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(data_size), &width, &height, &channels, 4);
    if (!pixels)
    {
        return false;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    *out_texture = texture;
    return true;
}

void CricleProgress(const char* name, float progress, float max, float radius, const ImVec2& size)
{
    const float tickness = 3.f;
    static float position = 0.f;

    position = progress / max * 6.28f;

    ImGui::GetForegroundDrawList()->PathClear();
    ImGui::GetForegroundDrawList()->PathArcTo(ImGui::GetCursorScreenPos() + size, radius, 0.f, 2.f * IM_PI, 120.f);
    ImGui::GetForegroundDrawList()->PathStroke(ImGui::GetColorU32(c::elements::background_widget), 0, tickness);

    ImGui::GetForegroundDrawList()->PathClear();
    ImGui::GetForegroundDrawList()->PathArcTo(ImGui::GetCursorScreenPos() + size, radius, IM_PI * 1.5f, IM_PI * 1.5f + position, 120.f);
    ImGui::GetForegroundDrawList()->PathStroke(ImGui::GetColorU32(c::main_color), 0, tickness);

    int procent = progress / (int)max * 100;

    std::string procent_str = std::to_string(procent) + "%";
    (void)name;
    (void)procent_str;
}

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD) != 0)
    {
        return 1;
    }

    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "Dear ImGui SDL3 OpenGL3 Example",
        1920,
        1080,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Inter_S = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 17.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    Inter_S_1 = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 18.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    Inter_S_2 = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 23.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    Inter_S_3 = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 15.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    Inter_B = io.Fonts->AddFontFromMemoryTTF(&Inter_Bold, sizeof Inter_Bold, 34.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    Icon = io.Fonts->AddFontFromMemoryTTF(&Icon_Pack, sizeof Icon_Pack, 26.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    Icon_Arrow = io.Fonts->AddFontFromMemoryTTF(&Arrow, sizeof Arrow, 6.f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

    LoadTextureFromMemory(Background, sizeof(Background), &bg);
    LoadTextureFromMemory(settings_checkbox, sizeof(settings_checkbox), &image::settings_checkbox);
    LoadTextureFromMemory(user_icon, sizeof(user_icon), &image::user);
    LoadTextureFromMemory(fps_icon, sizeof(fps_icon), &image::fps);
    LoadTextureFromMemory(ms_icon, sizeof(ms_icon), &image::ms);
    LoadTextureFromMemory(Logo_icon, sizeof(Logo_icon), &image::logo);

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            {
                done = true;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        {
            ImGuiContext& g = *GImGui;
            ImGuiStyle* style = &ImGui::GetStyle();
            (void)g;

            if (bg != 0)
            {
                ImGui::GetBackgroundDrawList()->AddImage(ToImTextureID(bg), ImVec2(0, 0), ImVec2(1920, 1080));
            }

            CustomStyleColor();

            ImGui::SetNextWindowSize(ImVec2(367, 50));
            ImGui::SetNextWindowPos({ 10, 10 });
            ImGui::Begin("##watermark", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
            {
                const auto& p = ImGui::GetWindowPos();

                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 16, p.y + 16), ImGui::GetColorU32(c::main_color), "TRUST");
                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 65, p.y + 16), ImGui::GetColorU32(c::text_active), "CODE");

                if (image::user != 0)
                {
                    ImGui::GetWindowDrawList()->AddImage(ToImTextureID(image::user), ImVec2(p.x + 120, p.y + 15), ImVec2(p.x + 140, p.y + 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::main_color));
                }

                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 144, p.y + 16), ImGui::GetColorU32(c::text_active), "Ostin");

                if (image::fps != 0)
                {
                    ImGui::GetWindowDrawList()->AddImage(ToImTextureID(image::fps), ImVec2(p.x + 198, p.y + 15), ImVec2(p.x + 218, p.y + 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::main_color));
                }

                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 222, p.y + 16), ImGui::GetColorU32(c::text_active), "350");

                ImGui::GetWindowDrawList()->AddText(Inter_S_3, 15.f, ImVec2(p.x + 251, p.y + 18), ImGui::GetColorU32(c::text_in_active), "FPS");

                if (image::ms != 0)
                {
                    ImGui::GetWindowDrawList()->AddImage(ToImTextureID(image::ms), ImVec2(p.x + 289, p.y + 15), ImVec2(p.x + 309, p.y + 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::main_color));
                }

                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 313, p.y + 16), ImGui::GetColorU32(c::text_active), "30");

                ImGui::GetWindowDrawList()->AddText(Inter_S_3, 15.f, ImVec2(p.x + 333, p.y + 18), ImGui::GetColorU32(c::text_in_active), "MS");
            }
            ImGui::End();

            ImGui::SetNextWindowSize(ImVec2(1050, 650));
            ImGui::Begin("General", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
            {
                const auto& p = ImGui::GetWindowPos();
                const ImVec2& region = ImGui::GetContentRegionMax();

                tab_alpha = ImLerp(tab_alpha, (page == active_tab) ? 1.f : 0.f, 18.f * ImGui::GetIO().DeltaTime);
                if (tab_alpha < 0.01f && tab_add < 0.01f)
                    active_tab = page;

                ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 250, p.y + region.y), ImGui::GetColorU32(c::child_rect), 12.f, ImDrawFlags_RoundCornersLeft);
                ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 250, p.y + region.y), ImGui::GetColorU32(c::child_rect), 12.f, ImDrawFlags_RoundCornersLeft);

                ImGui::GetWindowDrawList()->AddShadowCircle(ImVec2(p.x + 45, p.y + 50), 60.f, ImGui::GetColorU32(c::main_color), 170.f, ImVec2(0, 0));
                ImGui::GetWindowDrawList()->AddShadowCircle(ImVec2(p.x + region.x - 45, p.y + region.y - 45), 60.f, ImGui::GetColorU32(c::main_color), 170.f, ImVec2(0, 0));

                ImGui::GetWindowDrawList()->AddText(Inter_B, 34.f, ImVec2(p.x + 27, p.y + 32), ImGui::GetColorU32(c::main_color), "TRUST");
                ImGui::GetWindowDrawList()->AddText(Inter_B, 34.f, ImVec2(p.x + 125, p.y + 32), ImGui::GetColorU32(c::text_active), "CODE");

                ImGui::SetCursorPos(ImVec2(8, 112));
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));

                    if (ImGui::Tab("AimBot", "A", 0 == page, ImVec2(234, 50)))
                        page = 0;

                    if (ImGui::Tab("LegitBot", "L", 1 == page, ImVec2(234, 50)))
                        page = 1;

                    if (ImGui::Tab("Visuals", "V", 2 == page, ImVec2(234, 50)))
                        page = 2;

                    if (ImGui::Tab("Settings", "S", 3 == page, ImVec2(234, 50)))
                        page = 3;

                    if (ImGui::Tab("Config", "C", 4 == page, ImVec2(234, 50)))
                        page = 4;

                    ImGui::PopStyleVar();
                }
                ImGui::EndGroup();

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * style->Alpha);
                {
                    anim_text = ImLerp(anim_text, page == active_tab ? 20.f : 0.f, 14.f * ImGui::GetIO().DeltaTime);

                    if (active_tab == 0)
                    {
                        ImGui::GetWindowDrawList()->AddText(Inter_S_2, 23.f, ImVec2(p.x + 246 + anim_text, p.y + 18), ImGui::GetColorU32(c::text_active), "[Aimbot]");

                        ImGui::SetCursorPos(ImVec2(266, 76));
                        ImGui::BeginChild("tab1", ImVec2(376, 154), false);
                        {
                            static int select2 = 0;
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(266, 246));
                        ImGui::BeginChild("tab2", ImVec2(376, 276), false);
                        {
                            static bool Test = false;
                            ImGui::Checkbox("Through Walls", &Test);

                            static bool BulletTracer = true;
                            ImGui::Checkbox("Bullet Tracer", &BulletTracer);
                            static float color[4] = { 0 / 255.f, 149 / 255.f, 255 / 255.f };
                            ImGui::ColorEdit4("Bullet Color", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int = 60;
                            ImGui::SliderInt("I-G Radar Distance", &slider_int, 1, 240, "%du%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 76));
                        ImGui::BeginChild("tab3", ImVec2(376, 281), false);
                        {
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 373));
                        ImGui::BeginChild("tab4", ImVec2(376, 220), false);
                        {
                            static bool OffscreenESP = false;
                            ImGui::Checkbox("Offscreen ESP", &OffscreenESP);
                            static bool DynamicBoxes = true;
                            ImGui::Checkbox("Dynamic Boxes", &DynamicBoxes);
                            static float color2[4] = { 0 / 255.f, 255 / 255.f, 55 / 255.f };
                            ImGui::ColorEdit4("Dynamic Boxes Color", color2, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static bool DormantESP = true;
                            ImGui::Checkbox("Dormant ESP", &DormantESP);
                        }
                        ImGui::EndChild();
                    }

                    if (active_tab == 1)
                    {
                        ImGui::GetWindowDrawList()->AddText(Inter_S_2, 23.f, ImVec2(p.x + 246 + anim_text, p.y + 18), ImGui::GetColorU32(c::text_active), "[LegitBot]");

                        ImGui::SetCursorPos(ImVec2(266, 76));
                        ImGui::BeginChild("tab1", ImVec2(376, 154), false);
                        {
                            static int select2 = 0;
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(266, 246));
                        ImGui::BeginChild("tab2", ImVec2(376, 276), false);
                        {
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 76));
                        ImGui::BeginChild("tab3", ImVec2(376, 281), false);
                        {
                            static bool Test = false;
                            ImGui::Checkbox("Through Walls", &Test);

                            static bool BulletTracer = true;
                            ImGui::Checkbox("Bullet Tracer", &BulletTracer);
                            static float color[4] = { 0 / 255.f, 149 / 255.f, 255 / 255.f };
                            ImGui::ColorEdit4("Bullet Color", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int = 60;
                            ImGui::SliderInt("I-G Radar Distance", &slider_int, 1, 240, "%du%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 373));
                        ImGui::BeginChild("tab4", ImVec2(376, 220), false);
                        {
                            static bool OffscreenESP = false;
                            ImGui::Checkbox("Offscreen ESP", &OffscreenESP);
                            static bool DynamicBoxes = true;
                            ImGui::Checkbox("Dynamic Boxes", &DynamicBoxes);
                            static float color2[4] = { 0 / 255.f, 255 / 255.f, 55 / 255.f };
                            ImGui::ColorEdit4("Dynamic Boxes Color", color2, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static bool DormantESP = true;
                            ImGui::Checkbox("Dormant ESP", &DormantESP);
                        }
                        ImGui::EndChild();
                    }

                    if (active_tab == 2)
                    {
                        ImGui::GetWindowDrawList()->AddText(Inter_S_2, 23.f, ImVec2(p.x + 246 + anim_text, p.y + 18), ImGui::GetColorU32(c::text_active), "[Visuals]");

                        ImGui::SetCursorPos(ImVec2(266, 76));
                        ImGui::BeginChild("tab1", ImVec2(376, 154), false);
                        {
                            static int select2 = 0;
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(266, 246));
                        ImGui::BeginChild("tab2", ImVec2(376, 276), false);
                        {
                            static bool Test = false;
                            ImGui::Checkbox("Through Walls", &Test);

                            static bool BulletTracer = true;
                            ImGui::Checkbox("Bullet Tracer", &BulletTracer);
                            static float color[4] = { 0 / 255.f, 149 / 255.f, 255 / 255.f };
                            ImGui::ColorEdit4("Bullet Color", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int = 60;
                            ImGui::SliderInt("I-G Radar Distance", &slider_int, 1, 240, "%du%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 76));
                        ImGui::BeginChild("tab3", ImVec2(376, 281), false);
                        {
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 373));
                        ImGui::BeginChild("tab4", ImVec2(376, 220), false);
                        {
                            static bool OffscreenESP = false;
                            ImGui::Checkbox("Offscreen ESP", &OffscreenESP);
                            static bool DynamicBoxes = true;
                            ImGui::Checkbox("Dynamic Boxes", &DynamicBoxes);
                            static float color2[4] = { 0 / 255.f, 255 / 255.f, 55 / 255.f };
                            ImGui::ColorEdit4("Dynamic Boxes Color", color2, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static bool DormantESP = true;
                            ImGui::Checkbox("Dormant ESP", &DormantESP);
                        }
                        ImGui::EndChild();
                    }

                    if (active_tab == 3)
                    {
                        ImGui::GetWindowDrawList()->AddText(Inter_S_2, 23.f, ImVec2(p.x + 246 + anim_text, p.y + 18), ImGui::GetColorU32(c::text_active), "[Settings]");

                        ImGui::SetCursorPos(ImVec2(266, 76));
                        ImGui::BeginChild("tab1", ImVec2(376, 154), false);
                        {
                            static bool Test = false;
                            ImGui::Checkbox("Through Walls", &Test);

                            static bool BulletTracer = true;
                            ImGui::Checkbox("Bullet Tracer", &BulletTracer);

                            static int slider_int = 60;
                            ImGui::SliderInt("I-G Radar Distance", &slider_int, 1, 240, "%du%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(266, 246));
                        ImGui::BeginChild("tab2", ImVec2(376, 276), false);
                        {
                            static int select2 = 0;
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);
                            static float color[4] = { 0 / 255.f, 149 / 255.f, 255 / 255.f };
                            ImGui::ColorEdit4("Bullet Color", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 76));
                        ImGui::BeginChild("tab3", ImVec2(376, 281), false);
                        {
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift", "Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");
                        }
                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 373));
                        ImGui::BeginChild("tab4", ImVec2(376, 220), false);
                        {
                            static bool OffscreenESP = false;
                            ImGui::Checkbox("Offscreen ESP", &OffscreenESP);
                            static bool DynamicBoxes = true;
                            ImGui::Checkbox("Dynamic Boxes", &DynamicBoxes);
                            static float color2[4] = { 0 / 255.f, 255 / 255.f, 55 / 255.f };
                            ImGui::ColorEdit4("Dynamic Boxes Color", color2, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static bool DormantESP = true;
                            ImGui::Checkbox("Dormant ESP", &DormantESP);
                        }
                        ImGui::EndChild();
                    }
                }
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }

        ImGui::Render();

        int display_w = 0;
        int display_h = 0;
        SDL_GetWindowSizeInPixels(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    DestroyTexture(&bg);
    DestroyTexture(&image::settings_checkbox);
    DestroyTexture(&image::user);
    DestroyTexture(&image::fps);
    DestroyTexture(&image::ms);
    DestroyTexture(&image::logo);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}