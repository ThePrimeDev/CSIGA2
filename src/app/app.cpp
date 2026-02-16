#include "app.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <csignal>
#include <random>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl3.h>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include "platform/x11_helpers.h"
#include "platform/input.h"
#include "ui/menu.h"
#include "ui/assets.h"
#include "ui/state.h"
#include "ui/theme.h"
#include "functionalities/visuals/see-trough/esp.h"
#include "functionalities/aimbot/aimbot.h"
#include "functionalities/radar/radar.h"
#include "functionalities/clickity/clickity.h"
#include "audio/mvp_music.h"
#ifdef CSIGA2_FUN
#include "audio/virtual_mic.h"
#include "video/video_player.h"
#endif

namespace {

memory::Vec3 AngleToDirection(const memory::Vec2& angles) {
  constexpr float kPi = 3.14159265358979323846f;
  float pitch = angles.x * (kPi / 180.0f);
  float yaw = angles.y * (kPi / 180.0f);

  float cos_pitch = std::cos(pitch);
  return memory::Vec3(
      std::cos(yaw) * cos_pitch,
      std::sin(yaw) * cos_pitch,
      -std::sin(pitch));
}

// Per-weapon throw velocity from CS2 weapon data (CCSWeaponBaseVData::m_flThrowVelocity)
float GetWeaponThrowVelocity(memory::Weapon weapon) {
  switch (weapon) {
    case memory::Weapon::HeGrenade:   return 750.0f;
    case memory::Weapon::Flashbang:   return 750.0f;
    case memory::Weapon::Smoke:       return 750.0f;
    case memory::Weapon::Decoy:       return 750.0f;
    case memory::Weapon::Molotov:     return 400.0f;
    case memory::Weapon::Incendiary:  return 400.0f;
    default:                          return 750.0f;
  }
}

memory::Vec3 CalculateThrowVelocity(const memory::Vec2& view_angles,
                                    float throw_strength,
                                    const memory::Vec3& player_velocity,
                                    memory::Weapon weapon) {
  memory::Vec2 adjusted = view_angles;
  if (adjusted.x < -89.0f) {
    adjusted.x += 360.0f;
  } else if (adjusted.x > 89.0f) {
    adjusted.x -= 360.0f;
  }

  // Source 2 pitch adjustment: lift the throw angle slightly
  adjusted.x -= (90.0f - std::fabs(adjusted.x)) * 10.0f / 90.0f;
  memory::Vec3 direction = AngleToDirection(adjusted);

  // Per-weapon throw velocity formula (from Fatality/Source engine):
  // clamp(weapon_throw_velocity * 0.9, 15, 750) * (throwStrength * 0.7 + 0.3)
  float base_velocity = GetWeaponThrowVelocity(weapon) * 0.9f;
  base_velocity = std::max(15.0f, std::min(base_velocity, 750.0f));
  float strength = base_velocity * (throw_strength * 0.7f + 0.3f);
  memory::Vec3 velocity = direction * strength;

  // Add player velocity contribution (Source 2: 1.25x player velocity)
  return velocity + player_velocity * 1.25f;
}

// Detonation time limits for timed grenades (in seconds)
float GetGrenadeDetonationTime(memory::Weapon weapon) {
  switch (weapon) {
    case memory::Weapon::HeGrenade:   return 1.5f;
    case memory::Weapon::Flashbang:   return 1.5f;
    case memory::Weapon::Molotov:     return 3.5f;   // molotov_throw_detonate_time
    case memory::Weapon::Incendiary:  return 3.5f;
    default:                          return 60.0f;   // smoke/decoy use velocity check
  }
}

struct GrenadeTrajectory {
  std::vector<memory::Vec3> path;
  memory::Vec3 endpoint;
};

// Simulate grenade trajectory using Source engine grenade physics.
// Key insight from Fatality.win-Source: grenades use sv_gravity * 0.4 (40% gravity!)
// and trapezoidal integration for Z velocity.
// Without map geometry (ray tracing), we can't simulate bouncing.
GrenadeTrajectory SimulateGrenadeTrajectory(const memory::Vec3& start,
                                            const memory::Vec3& velocity,
                                            memory::Weapon weapon) {
  constexpr float kGravity = 800.0f;           // sv_gravity default
  constexpr float kGrenadeGravity = kGravity * 0.4f;  // grenades use 40% gravity!
  constexpr float kStep = 1.0f / 64.0f;       // Source 2 tick interval
  constexpr int kMaxSteps = 512;               // ~8s max simulation

  float detonation_time = GetGrenadeDetonationTime(weapon);
  int max_steps_for_time = static_cast<int>(detonation_time / kStep);
  int steps = std::min(kMaxSteps, max_steps_for_time);

  bool is_smoke = (weapon == memory::Weapon::Smoke);
  bool is_decoy = (weapon == memory::Weapon::Decoy);

  GrenadeTrajectory result;
  result.path.reserve(steps + 1);

  memory::Vec3 pos = start;
  memory::Vec3 vel = velocity;

  for (int i = 0; i < steps; ++i) {
    result.path.emplace_back(pos);

    // Source engine grenade physics:
    // new_vel_z = vel.z - (sv_gravity * 0.4) * dt
    // move = Vec3(vel.x * dt, vel.y * dt, (vel.z + new_vel_z) / 2 * dt)
    // This is trapezoidal integration matching CBaseCSGrenadeProjectile::PhysicsSimulate
    float new_vel_z = vel.z - kGrenadeGravity * kStep;
    memory::Vec3 move(
        vel.x * kStep,
        vel.y * kStep,
        (vel.z + new_vel_z) / 2.0f * kStep
    );
    vel.z = new_vel_z;
    pos = pos + move;

    // Smoke: detonates when velocity drops to nearly zero
    if (is_smoke) {
      float speed_sq = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;
      if (speed_sq <= 0.01f) break;
    }

    // Decoy: detonates when velocity drops to nearly zero
    if (is_decoy) {
      float speed_sq = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;
      if (speed_sq <= 0.04f) break;
    }
  }

  result.path.emplace_back(pos);
  result.endpoint = pos;
  return result;
}



}  // namespace

namespace {

std::atomic<bool> g_exit_requested(false);

void HandleSignal(int signal) {
  (void)signal;
  g_exit_requested.store(true);
}

bool HintARGBVisual() {
#ifdef __linux__
  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    return false;
  }

  XVisualInfo vinfo;
  if (XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor,
                       &vinfo) != 0) {
    char visualid[32];
    std::snprintf(visualid, sizeof(visualid), "%lu",
                  static_cast<unsigned long>(vinfo.visualid));
    SDL_SetHint(SDL_HINT_VIDEO_X11_WINDOW_VISUALID, visualid);
    XCloseDisplay(display);
    return true;
  }

  XCloseDisplay(display);
  return false;
#else
  return false;
#endif
}

bool UpdateEspManagerThrottled() {
  float current_time = static_cast<float>(ImGui::GetTime());
  return esp::UpdateEspThrottled(current_time);
}

const char* BombSiteLabel(int32_t site) {
  switch (site) {
    case 0:
      return "A";
    case 1:
      return "B";
    default:
      return "?";
  }
}

void ClearARGBVisualHint() {
#ifdef __linux__
  SDL_SetHint(SDL_HINT_VIDEO_X11_WINDOW_VISUALID, "");
#endif
}

bool CreateWindowAndContext(Uint64 window_flags,
                            int alpha_size,
                            bool use_argb_hint,
                            SDL_Window **out_window,
                            SDL_GLContext *out_context) {
  SDL_GL_ResetAttributes();
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                      SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alpha_size);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

  if (use_argb_hint) {
    if (!HintARGBVisual()) {
      SDL_Log("No ARGB visual found; transparency may be disabled.");
    }
  } else {
    ClearARGBVisualHint();
  }

  SDL_Window *window = SDL_CreateWindow(
      "Counter-Strike Intelligent Game Assistant 2",
      1920,
      1080,
      static_cast<SDL_WindowFlags>(window_flags));

  if (window) {
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
  }

  if (!window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    return false;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    return false;
  }

  *out_window = window;
  *out_context = gl_context;
  return true;
}

}  // namespace

int App::Run() {
  SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }


  Uint64 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS |
                        SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_TRANSPARENT | SDL_WINDOW_HIDDEN;

  SDL_Window *window = nullptr;
  SDL_GLContext gl_context = nullptr;

#ifdef __linux__
  HotkeyState hotkey_state = SetupGlobalHotkey();
#endif

  if (!CreateWindowAndContext(window_flags, 8, true, &window, &gl_context)) {
    SDL_Log("Retrying without ARGB visual hint.");
    if (!CreateWindowAndContext(window_flags, 8, false, &window, &gl_context)) {
      SDL_Log("Retrying without alpha.");
      if (!CreateWindowAndContext(window_flags, 0, false, &window,
                                  &gl_context)) {
        SDL_Quit();
        return 1;
      }
    }
  }

#ifdef __linux__
  // Set window type to DOCK to bypass GNOME shell reserved areas (top panel)
  SetOverrideFullscreen(window);
#endif

  SDL_ShowWindow(window);
  SDL_GL_SetSwapInterval(0);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ApplyTheme();
  LoadUiAssets();

  ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL2_Init();

#ifdef CSIGA2_FUN
  std::vector<audio::MicDevice> mic_devices = audio::ListPhysicalMics();
  std::string default_mic;
  if (!mic_devices.empty()) {
    default_mic = mic_devices.front().name;
  }
  audio::InitVirtualMic(default_mic);
#endif

  bool running = true;
  bool ui_visible = true;
#ifdef CSIGA2_FUN
  bool virtual_mic_shutdown = false;
#endif
  MenuState state;
#ifdef CSIGA2_FUN
  video::VideoPlayer video_player;
  bool video_panel_last_ui_visible = false;
  std::mt19937 video_rng(std::random_device{}());
  std::uniform_real_distribution<float> video_chance_dist(0.0f, 1.0f);
  double last_video_time = -1000.0;
  double next_video_check = 0.0;
#endif
  platform::Keyboard *keyboard = platform::Keyboard::Get();
  bool mvp_key_held = false;

  auto ToggleOverlay = [&](void) {
    ui_visible = !ui_visible;
    SetInteractive(window, ui_visible);
    if (ui_visible) {
      SDL_RaiseWindow(window);
    }
  };

  SetInteractive(window, ui_visible);

#ifdef CSIGA2_FUN
  auto ShutdownVirtualMicOnce = [&]() {
    if (!virtual_mic_shutdown) {
      audio::ShutdownVirtualMic();
      virtual_mic_shutdown = true;
    }
  };
#endif

  while (running) {
    if (g_exit_requested.load()) {
      running = false;
    }
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
          event.window.windowID == SDL_GetWindowID(window)) {
        running = false;
      }
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_INSERT) {
        ToggleOverlay();
      }
    }

#ifdef __linux__
    if (ConsumeGlobalHotkey(hotkey_state)) {
      ToggleOverlay();
    }
#endif

#ifdef CSIGA2_FUN
    if (g_video_play_request) {
      g_video_play_request = false;
      if (video_player.PlayRandomFromDir("assets/videos")) {
        double now = ImGui::GetTime();
        last_video_time = now;
        next_video_check = now + 15.0;
      }
    }

    if (g_video_random_enabled && !video_player.IsPlaying()) {
      double now = ImGui::GetTime();
      if (now >= next_video_check) {
        next_video_check = now + 1.0;
        if (now - last_video_time >= 15.0) {
          if (video_chance_dist(video_rng) <= 0.2f) {
            if (video_player.PlayRandomFromDir("assets/videos")) {
              last_video_time = now;
              next_video_check = now + 15.0;
            }
          }
        }
      }
    }
    if (!g_video_random_enabled) {
      next_video_check = ImGui::GetTime() + 1.0;
    }
#endif

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (g_see_through_walls) {
      ImGuiIO &io = ImGui::GetIO();
      ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
      ImGui::SetNextWindowSize(io.DisplaySize);
      ImGui::Begin("##see_trough_overlay", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                       ImGuiWindowFlags_NoBackground);
      esp::RenderEsp(ImGui::GetWindowPos(), ImGui::GetWindowSize());
      ImGui::End();
    }

    // Update Clickity (Triggerbot)
    functionalities::Clickity::Get().Update();

    // Update Aimbot
    functionalities::Aimbot::Get().Update();

    // Draw Radar
    functionalities::DrawRadar(ui_visible);

    if (g_grenade_pred) {
      if (UpdateEspManagerThrottled()) {
        const auto& process_opt = esp::g_esp_manager.getProcess();
        const auto& offsets_opt = esp::g_esp_manager.getOffsets();
        if (process_opt.has_value() && offsets_opt.has_value()) {
          const auto& process = process_opt.value();
          const auto& offsets = offsets_opt.value();
          auto local_player_opt = memory::Player::localPlayer(process, offsets);
          if (local_player_opt.has_value()) {
            const auto& local_player = local_player_opt.value();
            if (local_player.isValid(process, offsets)) {
              memory::Weapon weapon = local_player.weapon(process, offsets);
              bool is_grenade = (weapon == memory::Weapon::Flashbang ||
                                 weapon == memory::Weapon::HeGrenade ||
                                 weapon == memory::Weapon::Smoke ||
                                 weapon == memory::Weapon::Molotov ||
                                 weapon == memory::Weapon::Decoy ||
                                 weapon == memory::Weapon::Incendiary);
              if (is_grenade) {
                const memory::Vec2 view_angles = local_player.viewAngles(process, offsets);
                const memory::Vec3 direction = AngleToDirection(view_angles);
                const memory::Vec3 eye_pos = local_player.eyePosition(process, offsets);
                const memory::Vec3 player_vel = local_player.velocity(process, offsets);
                constexpr float kDefaultThrowStrength = 1.0f;  // Full throw (can't read game mouse state externally)

                // Start position: eye + player_vel*0.1 forward, offset Z by throw strength
                const memory::Vec3 adjusted_eye(
                    eye_pos.x + player_vel.x * 0.1f,
                    eye_pos.y + player_vel.y * 0.1f,
                    eye_pos.z + player_vel.z * 0.1f + (kDefaultThrowStrength * 12.0f - 12.0f)
                );
                // Trace forward 22 units, then back up 6 (approximation without actual trace)
                const memory::Vec3 start = adjusted_eye + direction * 16.0f;
                const memory::Vec3 velocity = CalculateThrowVelocity(view_angles, kDefaultThrowStrength,
                                                                    player_vel, weapon);

                const auto trajectory = SimulateGrenadeTrajectory(start, velocity, weapon);
                const auto& view_matrix = esp::g_esp_manager.getViewMatrix();
                ImVec2 screen_size = ImVec2(static_cast<float>(esp::g_esp_manager.getWindowSize().x),
                                            static_cast<float>(esp::g_esp_manager.getWindowSize().y));
                if (screen_size.x <= 1.0f || screen_size.y <= 1.0f) {
                  ImGuiIO &io = ImGui::GetIO();
                  screen_size = io.DisplaySize;
                }
                ImDrawList* draw = ImGui::GetForegroundDrawList();
                ImU32 color = ImGui::ColorConvertFloat4ToU32(g_grenade_pred_color);

                // Draw trajectory line
                for (size_t i = 1; i < trajectory.path.size(); ++i) {
                  auto p0 = esp::WorldToScreen(trajectory.path[i - 1], view_matrix, screen_size);
                  auto p1 = esp::WorldToScreen(trajectory.path[i], view_matrix, screen_size);
                  if (!p0.has_value() || !p1.has_value()) continue;
                  draw->AddLine(*p0, *p1, color, 2.0f);
                }

                // Draw endpoint marker (filled circle at detonation/landing point)
                auto endpoint_screen = esp::WorldToScreen(trajectory.endpoint, view_matrix, screen_size);
                if (endpoint_screen.has_value()) {
                  draw->AddCircleFilled(*endpoint_screen, 6.0f, color, 16);
                  draw->AddCircle(*endpoint_screen, 8.0f, IM_COL32(0, 0, 0, 200), 16, 2.0f);
                }
              }
            }
          }
        }
      }
    }

    // Grenade Proximity Warning overlay
    if (g_grenade_proximity_warning) {
      if (UpdateEspManagerThrottled()) {
        const auto& grenades = esp::g_esp_manager.getGrenadeProximity();
        if (!grenades.empty()) {
          const auto& view_matrix = esp::g_esp_manager.getViewMatrix();
          ImVec2 screen_size = ImVec2(static_cast<float>(esp::g_esp_manager.getWindowSize().x),
                                      static_cast<float>(esp::g_esp_manager.getWindowSize().y));
          if (screen_size.x <= 1.0f || screen_size.y <= 1.0f) {
            ImGuiIO &io = ImGui::GetIO();
            screen_size = io.DisplaySize;
          }
          ImDrawList* draw = ImGui::GetForegroundDrawList();

          for (const auto& g : grenades) {
            if (g.type == esp::GrenadeProximityData::Inferno) {
              // Draw each fire point as a filled circle
              ImU32 fire_color = ImGui::ColorConvertFloat4ToU32(g_grenade_proximity_fire_color);
              for (const auto& fire_pos : g.fire_hull) {
                auto screen = esp::WorldToScreen(fire_pos, view_matrix, screen_size);
                if (!screen.has_value()) continue;
                draw->AddCircleFilled(*screen, 12.0f, fire_color, 16);
              }
              // Draw outline around the origin
              auto origin_screen = esp::WorldToScreen(g.position, view_matrix, screen_size);
              if (origin_screen.has_value()) {
                ImU32 outline = IM_COL32(255, 80, 0, 180);
                draw->AddCircle(*origin_screen, 16.0f, outline, 24, 2.0f);
                draw->AddText(ImVec2(origin_screen->x + 18.0f, origin_screen->y - 8.0f),
                              IM_COL32(255, 140, 0, 255), "FIRE");
              }
            } else if (g.type == esp::GrenadeProximityData::HE) {
              // HE grenade: draw a world-space circle on the ground plane
              // showing the actual blast radius (where you take damage)
              constexpr int kSegments = 32;
              constexpr float kTwoPi = 2.0f * 3.14159265358979323846f;
              ImU32 zone_color = ImGui::ColorConvertFloat4ToU32(g_grenade_proximity_color);
              ImU32 outline_color = IM_COL32(255, 100, 0, 220);

              std::vector<ImVec2> screen_points;
              screen_points.reserve(kSegments);
              for (int i = 0; i < kSegments; ++i) {
                float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(kSegments);
                memory::Vec3 world_pt;
                world_pt.x = g.position.x + std::cos(angle) * g.radius;
                world_pt.y = g.position.y + std::sin(angle) * g.radius;
                world_pt.z = g.position.z;  // same Z = ground plane
                auto sp = esp::WorldToScreen(world_pt, view_matrix, screen_size);
                if (sp.has_value()) {
                  screen_points.push_back(*sp);
                }
              }

              if (screen_points.size() >= 3) {
                // Filled polygon for danger zone
                draw->AddConvexPolyFilled(screen_points.data(),
                                          static_cast<int>(screen_points.size()), zone_color);
                // Outline
                draw->AddPolyline(screen_points.data(),
                                  static_cast<int>(screen_points.size()),
                                  outline_color, ImDrawFlags_Closed, 2.0f);
              }

              // Label at center
              auto center_screen = esp::WorldToScreen(g.position, view_matrix, screen_size);
              if (center_screen.has_value()) {
                const char* label = "HE";
                ImVec2 text_size = ImGui::CalcTextSize(label);
                draw->AddText(ImVec2(center_screen->x - text_size.x * 0.5f,
                                     center_screen->y - text_size.y * 0.5f),
                              IM_COL32(255, 255, 255, 255), label);
              }
            } else {
              // Flash / Smoke / Molotov / Decoy — screen-space overlay
              auto screen = esp::WorldToScreen(g.position, view_matrix, screen_size);
              if (!screen.has_value()) continue;

              ImU32 zone_color = ImGui::ColorConvertFloat4ToU32(g_grenade_proximity_color);

              // Approximate screen-space radius for the danger zone
              memory::Vec3 offset_pos = g.position;
              offset_pos.x += g.radius;
              auto offset_screen = esp::WorldToScreen(offset_pos, view_matrix, screen_size);
              float screen_radius = 30.0f;
              if (offset_screen.has_value()) {
                float dx = offset_screen->x - screen->x;
                float dy = offset_screen->y - screen->y;
                screen_radius = std::sqrt(dx * dx + dy * dy);
                if (screen_radius < 10.0f) screen_radius = 10.0f;
                if (screen_radius > 400.0f) screen_radius = 400.0f;
              }

              draw->AddCircleFilled(*screen, screen_radius, zone_color, 32);
              ImU32 outline = IM_COL32(255, 100, 0, 200);
              draw->AddCircle(*screen, screen_radius, outline, 32, 2.0f);

              const char* label = "GRENADE";
              switch (g.type) {
                case esp::GrenadeProximityData::Flash:    label = "FLASH"; break;
                case esp::GrenadeProximityData::Smoke:    label = "SMOKE"; break;
                case esp::GrenadeProximityData::Molotov:  label = "MOLOTOV"; break;
                case esp::GrenadeProximityData::Decoy:    label = "DECOY"; break;
                default: break;
              }
              ImVec2 text_size = ImGui::CalcTextSize(label);
              draw->AddText(ImVec2(screen->x - text_size.x * 0.5f, screen->y - text_size.y * 0.5f),
                            IM_COL32(255, 255, 255, 255), label);
            }
          }
        }
      }
    }

    // Sniper crosshair overlay (separate from ESP)
    if (g_sniper_crosshair) {
      if (UpdateEspManagerThrottled() &&
          esp::g_esp_manager.isLocalPlayerHoldingSniper()) {
        ImGuiIO &io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##crosshair_overlay", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoBackground);
        {
          ImDrawList* draw = ImGui::GetWindowDrawList();
          ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
          
          // Crosshair settings
          const float length = 8.0f;
          const float gap = 3.0f;
          const float thickness = 2.0f;
          const ImU32 color = IM_COL32(0, 255, 0, 255);  // Green
          const ImU32 outline = IM_COL32(0, 0, 0, 255);  // Black outline
          
          // Draw outline first
          draw->AddLine(ImVec2(center.x - length - gap, center.y), 
                        ImVec2(center.x - gap, center.y), outline, thickness + 2.0f);
          draw->AddLine(ImVec2(center.x + gap, center.y), 
                        ImVec2(center.x + length + gap, center.y), outline, thickness + 2.0f);
          draw->AddLine(ImVec2(center.x, center.y - length - gap), 
                        ImVec2(center.x, center.y - gap), outline, thickness + 2.0f);
          draw->AddLine(ImVec2(center.x, center.y + gap), 
                        ImVec2(center.x, center.y + length + gap), outline, thickness + 2.0f);
          
          // Draw crosshair lines
          draw->AddLine(ImVec2(center.x - length - gap, center.y), 
                        ImVec2(center.x - gap, center.y), color, thickness);
          draw->AddLine(ImVec2(center.x + gap, center.y), 
                        ImVec2(center.x + length + gap, center.y), color, thickness);
          draw->AddLine(ImVec2(center.x, center.y - length - gap), 
                        ImVec2(center.x, center.y - gap), color, thickness);
          draw->AddLine(ImVec2(center.x, center.y + gap), 
                        ImVec2(center.x, center.y + length + gap), color, thickness);
          
          // Center dot
          draw->AddCircleFilled(center, 1.5f, color, 8);
        }
        ImGui::End();
      }
    }

    if (g_headshoot_line) {
      if (UpdateEspManagerThrottled()) {
        ImGuiIO &io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##headshoot_line_overlay", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoBackground);
        {
          ImDrawList* draw = ImGui::GetWindowDrawList();
          ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
          memory::Vec2 view_angles = esp::g_esp_manager.getLocalViewAngles();
          float fov_deg = esp::g_esp_manager.getLocalFov();
          if (fov_deg <= 1.0f) {
            fov_deg = static_cast<float>(memory::cs2::DEFAULT_FOV);
          }

          constexpr float kPi = 3.14159265358979323846f;
          float fov_rad = fov_deg * (kPi / 180.0f);
          float pitch_rad = view_angles.x * (kPi / 180.0f);
          float denom = 2.0f * std::sin(fov_rad);
          if (std::abs(denom) > 0.0001f) {
            float offset = io.DisplaySize.y / denom * std::sin(pitch_rad);
            float pos_y = center.y - offset;

            ImU32 color = ImGui::ColorConvertFloat4ToU32(g_headshoot_line_color);
            ImU32 outline = IM_COL32(0, 0, 0, 200);

            ImVec2 left_min(center.x - 20.0f, pos_y);
            ImVec2 left_max(left_min.x + 17.0f, pos_y + 3.0f);
            ImVec2 right_min(center.x + 6.0f, pos_y);
            ImVec2 right_max(right_min.x + 17.0f, pos_y + 3.0f);

            draw->AddRectFilled(ImVec2(left_min.x - 1.0f, left_min.y - 1.0f),
                                ImVec2(left_max.x + 1.0f, left_max.y + 1.0f),
                                outline);
            draw->AddRectFilled(left_min, left_max, color);
            draw->AddRectFilled(ImVec2(right_min.x - 1.0f, right_min.y - 1.0f),
                                ImVec2(right_max.x + 1.0f, right_max.y + 1.0f),
                                outline);
            draw->AddRectFilled(right_min, right_max, color);
          }
        }
        ImGui::End();
      }
    }

    if (g_where_is_planted) {
      ImGuiIO &io = ImGui::GetIO();
      const float panel_width = 260.0f;
      const float panel_height = 120.0f;
      static bool last_ui_visible = false;
      if (g_bomb_panel_pos.x < 0.0f || g_bomb_panel_pos.y < 0.0f) {
        g_bomb_panel_pos = ImVec2(io.DisplaySize.x - panel_width - 10.0f, 10.0f);
      }
      if (!ui_visible) {
        ImGui::SetNextWindowPos(g_bomb_panel_pos, ImGuiCond_Always);
      } else if (!last_ui_visible) {
        ImGui::SetNextWindowPos(g_bomb_panel_pos, ImGuiCond_Always);
      }
      ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                               ImGuiWindowFlags_NoFocusOnAppearing;
      if (!ui_visible) {
        flags |= ImGuiWindowFlags_NoMove;
      }
      ImGui::Begin("##bomb_panel", nullptr, flags);
      if (ui_visible) {
        g_bomb_panel_pos = ImGui::GetWindowPos();
      }
      last_ui_visible = ui_visible;
      if (!UpdateEspManagerThrottled()) {
        ImGui::TextUnformatted("Waiting for CS2...");
      } else {
        const auto& bomb = esp::g_esp_manager.getBombData();
        if (!bomb.planted) {
          ImGui::TextUnformatted("Bomb not planted.");
        } else {
          char timer_buf[32];
          std::snprintf(timer_buf, sizeof(timer_buf), "%.2f", bomb.timer);
          ImGui::Text("Bomb site: %s", BombSiteLabel(bomb.bomb_site));
          ImGui::Text("Timer: %s", timer_buf);
          ImGui::Text("Defusing: %s", bomb.being_defused ? "yes" : "no");
          if (bomb.being_defused) {
            char defuse_buf[32];
            std::snprintf(defuse_buf, sizeof(defuse_buf), "%.2f", bomb.defuse_remain_time);
            ImGui::Text("Defuse: %s", defuse_buf);
          }
        }
      }
      ImGui::End();
    }

#ifdef CSIGA2_FUN
    bool video_render = false;
    ImVec2 video_render_pos(0.0f, 0.0f);
    ImVec2 video_render_size(0.0f, 0.0f);

    if (video_player.IsPlaying() || ui_visible) {
      ImGuiIO &io = ImGui::GetIO();
      const ImVec2 panel_size(450.0f, 200.0f);
      if (g_video_panel_pos.x < 0.0f || g_video_panel_pos.y < 0.0f) {
        g_video_panel_pos = ImVec2(10.0f, io.DisplaySize.y - panel_size.y - 10.0f);
      }

      if (!ui_visible) {
        ImGui::SetNextWindowPos(g_video_panel_pos, ImGuiCond_Always);
      } else if (!video_panel_last_ui_visible) {
        ImGui::SetNextWindowPos(g_video_panel_pos, ImGuiCond_Always);
      }

      ImGui::SetNextWindowSize(panel_size);
      ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                               ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBackground;
      if (!ui_visible) {
        flags |= ImGuiWindowFlags_NoMove;
      }

      ImGui::Begin("##video_panel", nullptr, flags);
      ImVec2 panel_pos = ImGui::GetWindowPos();
      ImVec2 panel_size_actual = ImGui::GetWindowSize();
      if (ui_visible) {
        g_video_panel_pos = panel_pos;
        ImDrawList *draw = ImGui::GetWindowDrawList();
        draw->AddRect(panel_pos, ImVec2(panel_pos.x + panel_size_actual.x,
                                        panel_pos.y + panel_size_actual.y),
                      IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
      }
      ImGui::End();

      video_panel_last_ui_visible = ui_visible;
      if (video_player.IsPlaying()) {
        video_render = true;
        video_render_pos = panel_pos;
        video_render_size = panel_size_actual;
      }
    } else {
      video_panel_last_ui_visible = ui_visible;
    }
#endif

    {
      static int32_t last_mvp_count = 0;
      static float congrats_until = 0.0f;
      float now = static_cast<float>(ImGui::GetTime());

      if (UpdateEspManagerThrottled()) {
        int32_t current_mvp = esp::g_esp_manager.getLocalMvpCount();
        if (current_mvp > last_mvp_count) {
          if (g_mvp_music_enabled) {
            audio::PlayMvpMusic(g_mvp_music_selection);
            congrats_until = 0.0f;
          } else {
            congrats_until = now + 4.0f;
          }
        }
        last_mvp_count = current_mvp;
      }

      if (now < congrats_until) {
        ImGuiIO &io = ImGui::GetIO();
        const char* msg = "CONGRATS";
        ImVec2 text_size = ImGui::CalcTextSize(msg);
        ImVec2 pos(io.DisplaySize.x * 0.5f - text_size.x * 0.5f,
                   io.DisplaySize.y * 0.5f - text_size.y * 0.5f);
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->AddText(pos, IM_COL32(255, 215, 0, 255), msg);
      }
    }

    {
      bool should_hold = audio::IsMvpMusicPlaying();
      if (keyboard && keyboard->IsValid()) {
        if (should_hold && !mvp_key_held) {
          keyboard->GravePress();
          mvp_key_held = true;
        } else if (!should_hold && mvp_key_held) {
          keyboard->GraveRelease();
          mvp_key_held = false;
        }
      } else {
        mvp_key_held = false;
      }
    }

    if (g_watermark_enabled) {
      RenderWatermark();
    }
    RenderStatusIndicator();
    if (ui_visible) {
      RenderMenu(state, &ui_visible, &running);
    }
    if (!running) {
#ifdef CSIGA2_FUN
      ShutdownVirtualMicOnce();
#endif
    }

    ImGui::Render();

    int display_width = 0;
    int display_height = 0;
    SDL_GetWindowSizeInPixels(window, &display_width, &display_height);
    glViewport(0, 0, display_width, display_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
#ifdef CSIGA2_FUN
    if (video_render) {
      video_player.Render(video_render_pos, video_render_size,
                          ImVec2(static_cast<float>(display_width),
                                 static_cast<float>(display_height)));
    }
#endif
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  UnloadUiAssets();
  if (mvp_key_held && keyboard && keyboard->IsValid()) {
    keyboard->GraveRelease();
    mvp_key_held = false;
  }
  audio::ShutdownMvpMusic();
#ifdef CSIGA2_FUN
  audio::ShutdownVirtualMic();
#endif

  SDL_GL_DestroyContext(gl_context);
  SDL_DestroyWindow(window);
  

  SDL_Quit();

#ifdef __linux__
  TeardownGlobalHotkey(hotkey_state);
#endif

  return 0;
}
