#include "game_data_thread.h"

#include <chrono>

#include "functionalities/aimbot/aimbot.h"
#include "functionalities/clickity/clickity.h"
#include "memory/memory.h"
#include "ui/state.h"

GameDataThread::GameDataThread() = default;

GameDataThread::~GameDataThread() {
  stop();
}

void GameDataThread::start() {
  if (running_.load()) return;
  running_.store(true);
  thread_ = std::thread(&GameDataThread::run, this);
}

void GameDataThread::stop() {
  running_.store(false);
  if (thread_.joinable()) {
    thread_.join();
  }
}

GameSnapshot GameDataThread::getSnapshot() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return snapshot_;
}

void GameDataThread::run() {
  float last_connect_attempt = 0.0f;
  float time_counter = 0.0f;

  while (running_.load()) {
    auto loop_start = std::chrono::steady_clock::now();
    time_counter += 0.01f;  // Approximate time counter for throttle

    // --- ESP connection & update ---
    if (!esp::g_esp_manager.isConnected()) {
      if (time_counter - last_connect_attempt > 2.0f) {
        last_connect_attempt = time_counter;
        esp::g_esp_manager.connect();
      }
    }

    bool connected = esp::g_esp_manager.isConnected();

    if (connected) {
      esp::g_esp_manager.update();
    }

    // --- Aimbot data reading ---
    functionalities::Aimbot::Get().ReadGameData();

    // --- Triggerbot data reading ---
    functionalities::Clickity::Get().ReadGameData();

    // --- Build snapshot ---
    {
      std::lock_guard<std::mutex> lock(mutex_);
      snapshot_.connected = connected;

      if (connected) {
        snapshot_.players = esp::g_esp_manager.getPlayers();
        snapshot_.grenadeProximity = esp::g_esp_manager.getGrenadeProximity();
        snapshot_.viewMatrix = esp::g_esp_manager.getViewMatrix();
        snapshot_.windowSize = esp::g_esp_manager.getWindowSize();
        snapshot_.localTeam = esp::g_esp_manager.getLocalTeam();
        snapshot_.localWeapon = esp::g_esp_manager.getLocalWeapon();
        snapshot_.localIsScoped = esp::g_esp_manager.isLocalPlayerScoped();
        snapshot_.localMvpCount = esp::g_esp_manager.getLocalMvpCount();
        snapshot_.localViewAngles = esp::g_esp_manager.getLocalViewAngles();
        snapshot_.localFov = esp::g_esp_manager.getLocalFov();
        snapshot_.bomb = esp::g_esp_manager.getBombData();

        // Read grenade prediction data from process
        const auto& process_opt = esp::g_esp_manager.getProcess();
        const auto& offsets_opt = esp::g_esp_manager.getOffsets();
        if (process_opt.has_value() && offsets_opt.has_value()) {
          const auto& process = process_opt.value();
          const auto& offsets = offsets_opt.value();
          auto local_player_opt = memory::Player::localPlayer(process, offsets);
          if (local_player_opt.has_value()) {
            const auto& lp = local_player_opt.value();
            snapshot_.hasLocalPlayer = true;
            snapshot_.localPlayerValid = lp.isValid(process, offsets);
            if (snapshot_.localPlayerValid) {
              snapshot_.localWeaponEnum = lp.weapon(process, offsets);
              snapshot_.localEyePos = lp.eyePosition(process, offsets);
              snapshot_.localVelocity = lp.velocity(process, offsets);
              snapshot_.localViewAnglesRaw = lp.viewAngles(process, offsets);
            }
          } else {
            snapshot_.hasLocalPlayer = false;
            snapshot_.localPlayerValid = false;
          }
        }
      } else {
        snapshot_.players.clear();
        snapshot_.grenadeProximity.clear();
        snapshot_.hasLocalPlayer = false;
        snapshot_.localPlayerValid = false;
      }
    }

    // --- Apply aimbot & triggerbot actions (mouse/key I/O, runs at bg thread rate) ---
    functionalities::Aimbot::Get().ApplyAim();
    functionalities::Clickity::Get().ApplyAction();

    // Target ~100 Hz
    auto loop_end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(loop_end - loop_start);
    if (elapsed.count() < 10) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10) - elapsed);
    }
  }
}
