// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "main.h"
#include "Font.h"
#include "image.h"
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include "imgui_settings.h"
#include "custom_widgets.h"
#include <D3DX11tex.h>
#pragma comment(lib, "D3DX11.lib")

static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int notify_select = 0;
const char* notify_items[2]{ "Circle", "Line" };
namespace image
{
   
    ID3D11ShaderResourceView* settings_checkbox = nullptr;
    ID3D11ShaderResourceView* user = nullptr;
    ID3D11ShaderResourceView* fps = nullptr;
    ID3D11ShaderResourceView* ms = nullptr;

    ID3D11ShaderResourceView* logo = nullptr;
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

}


D3DX11_IMAGE_LOAD_INFO info; ID3DX11ThreadPump* pump{ nullptr };

int main(int, char**)
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_POPUP, 0, 0, 1920, 1080, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Inter_S = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 17.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Inter_S_1 = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 18.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Inter_S_2 = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 23.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Inter_S_3 = io.Fonts->AddFontFromMemoryTTF(&Inter_SemmiBold, sizeof Inter_SemmiBold, 15.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Inter_B = io.Fonts->AddFontFromMemoryTTF(&Inter_Bold, sizeof Inter_Bold, 34.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Icon = io.Fonts->AddFontFromMemoryTTF(&Icon_Pack, sizeof Icon_Pack, 26.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    Icon_Arrow = io.Fonts->AddFontFromMemoryTTF(&Arrow, sizeof Arrow, 6.f, NULL, io.Fonts->GetGlyphRangesCyrillic());

    if (image::settings_checkbox == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, settings_checkbox, sizeof(settings_checkbox), &info, pump, &image::settings_checkbox, 0);

    if (image::user == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, user_icon, sizeof(user_icon), &info, pump, &image::user, 0);

    if (image::fps == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, fps_icon, sizeof(fps_icon), &info, pump, &image::fps, 0);

    if (image::ms == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, ms_icon, sizeof(ms_icon), &info, pump, &image::ms, 0);

    if (image::logo == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, Logo_icon, sizeof(Logo_icon), &info, pump, &image::logo, 0);

    bool done = false;
    while (!done)
    {

        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            ImGuiContext& g = *GImGui;
            ImGuiStyle* style = &ImGui::GetStyle();
            if (bg == nullptr) D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, Background, sizeof(Background), &info, pump, &bg, 0);
            ImGui::GetBackgroundDrawList()->AddImage(bg, ImVec2(0, 0), ImVec2(1920, 1080), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

            CustomStyleColor();

            ImGui::SetNextWindowSize(ImVec2(367, 50));
            ImGui::SetNextWindowPos({ 10, 10 });
            ImGui::Begin("##watermark", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
            {
                auto draw = ImGui::GetForegroundDrawList();
                const auto& p = ImGui::GetWindowPos();

                const ImVec2& region = ImGui::GetContentRegionMax();

                SYSTEMTIME st;
                GetLocalTime(&st);
           
                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 16, p.y + 16), ImGui::GetColorU32(c::main_color), "TRUST"); ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 65, p.y + 16), ImGui::GetColorU32(c::text_active), "CODE");

                ImGui::GetWindowDrawList()->AddImage(image::user, ImVec2(p.x + 120, p.y + 15), ImVec2(p.x + 140, p.y + 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::main_color));

                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 144, p.y + 16), ImGui::GetColorU32(c::text_active), "Ostin");

                ImGui::GetWindowDrawList()->AddImage(image::fps, ImVec2(p.x + 198, p.y + 15), ImVec2(p.x + 218, p.y + 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::main_color));

                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 222, p.y + 16), ImGui::GetColorU32(c::text_active), "350");

                ImGui::GetWindowDrawList()->AddText(Inter_S_3, 15.f, ImVec2(p.x + 251, p.y + 18), ImGui::GetColorU32(c::text_in_active), "FPS");

                ImGui::GetWindowDrawList()->AddImage(image::ms, ImVec2(p.x + 289, p.y + 15), ImVec2(p.x + 309, p.y + 35), ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(c::main_color));

                ImGui::GetWindowDrawList()->AddText(Inter_S, 17.f, ImVec2(p.x + 313, p.y + 16), ImGui::GetColorU32(c::text_active), "30");

                ImGui::GetWindowDrawList()->AddText(Inter_S_3, 15.f, ImVec2(p.x + 333, p.y + 18), ImGui::GetColorU32(c::text_in_active), "MS");

            }ImGui::End();


            ImGui::SetNextWindowSize(ImVec2(1050, 650));
            ImGui::Begin("General", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
            {
                auto draw = ImGui::GetWindowDrawList();

                const auto& p = ImGui::GetWindowPos();

                const ImVec2& region = ImGui::GetContentRegionMax();          

                tab_alpha = ImLerp(tab_alpha, (page == active_tab) ? 1.f : 0.f, 18.f * ImGui::GetIO().DeltaTime);
                if (tab_alpha < 0.01f && tab_add < 0.01f) active_tab = page;

                ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 250, p.y + region.y), ImGui::GetColorU32(c::child_rect), 12.f, ImDrawFlags_RoundCornersLeft);
              
                ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 250, p.y + region.y), ImGui::GetColorU32(c::child_rect), 12.f, ImDrawFlags_RoundCornersLeft);

                ImGui::GetWindowDrawList()->AddShadowCircle(ImVec2(p.x + 45, p.y + 50), 60.f, ImGui::GetColorU32(c::main_color), 170.f, ImVec2(0, 0));

                ImGui::GetWindowDrawList()->AddShadowCircle(ImVec2(p.x + region.x - 45, p.y + region.y - 45), 60.f, ImGui::GetColorU32(c::main_color), 170.f, ImVec2(0, 0));

                ImGui::GetWindowDrawList()->AddText(Inter_B, 34.f, ImVec2(p.x + 27, p.y + 32), ImGui::GetColorU32(c::main_color), "TRUST"); ImGui::GetWindowDrawList()->AddText(Inter_B, 34.f, ImVec2(p.x + 125, p.y + 32), ImGui::GetColorU32(c::text_active), "CODE");

                ImGui::SetCursorPos(ImVec2(8, 112));
                ImGui::BeginGroup();
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));

                    if (ImGui::Tab("AimBot", "A", 0 == page, ImVec2(234, 50))) page = 0;

                    if (ImGui::Tab("LegitBot", "L", 1 == page, ImVec2(234, 50))) page = 1;

                    if (ImGui::Tab("Visuals", "V", 2 == page, ImVec2(234, 50))) page = 2;

                    if (ImGui::Tab("Settings", "S", 3 == page, ImVec2(234, 50))) page = 3;

                    if (ImGui::Tab("Config", "C", 4 == page, ImVec2(234, 50))) page = 4;

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
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);


                        }ImGui::EndChild();

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

                        }ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 76));
                        ImGui::BeginChild("tab3", ImVec2(376, 281), false);
                        {                                                 
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");
                        }ImGui::EndChild();

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

                        }ImGui::EndChild();
                  
                }
                    if (active_tab == 1)
                    {

                       
                        ImGui::GetWindowDrawList()->AddText(Inter_S_2, 23.f, ImVec2(p.x + 246 + anim_text, p.y + 18), ImGui::GetColorU32(c::text_active), "[LegitBot]");

                        ImGui::SetCursorPos(ImVec2(266, 76));
                        ImGui::BeginChild("tab1", ImVec2(376, 154), false);
                        {
                            static int select2 = 0;
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);


                        }ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(266, 246));
                        ImGui::BeginChild("tab2", ImVec2(376, 276), false);
                        {            
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");

                        }ImGui::EndChild();

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


                           
                        }ImGui::EndChild();

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

                        }ImGui::EndChild();

                    }
                    if (active_tab == 2)
                    {


                        ImGui::GetWindowDrawList()->AddText(Inter_S_2, 23.f, ImVec2(p.x + 246 + anim_text, p.y + 18), ImGui::GetColorU32(c::text_active), "[Visuals]");

                        ImGui::SetCursorPos(ImVec2(266, 76));
                        ImGui::BeginChild("tab1", ImVec2(376, 154), false);
                        {
                            static int select2 = 0;
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);


                        }ImGui::EndChild();

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

                        }ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 76));
                        ImGui::BeginChild("tab3", ImVec2(376, 281), false);
                        {
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");
                        }ImGui::EndChild();

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

                        }ImGui::EndChild();

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

                        }ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(266, 246));
                        ImGui::BeginChild("tab2", ImVec2(376, 276), false);
                        {
                            static int select2 = 0;
                            const char* items2[4]{ "Left Alt", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select2, items2, IM_ARRAYSIZE(items2), 2);
                            static bool Enabled = true;
                            ImGui::Checkbox("Enabled", &Enabled);
                            static float color[4] = { 0 / 255.f, 149 / 255.f, 255 / 255.f };
                            ImGui::ColorEdit4("Bullet Color", color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                          

                        }ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2(658, 76));
                        ImGui::BeginChild("tab3", ImVec2(376, 281), false);
                        {
                            static bool Chams = true;
                            ImGui::Checkbox("Enabled Chams", &Chams);
                            static int select3 = 0;
                            const char* items3[4]{ "Glow Outline", "Right Alt", "Left Shift","Right Shift" };
                            ImGui::Combo("Type", &select3, items3, IM_ARRAYSIZE(items3), 2);
                            static float color1[4] = { 255 / 255.f, 0 / 255.f, 0 / 255.f };
                            ImGui::ColorEdit4("Color", color1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                            static int slider_int1 = 60;
                            ImGui::SliderInt("Distance", &slider_int1, 1, 240, "%ds%");
                        }ImGui::EndChild();

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

                        }ImGui::EndChild();

                    }
                }ImGui::PopStyleVar();

            }ImGui::End();
        }
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
