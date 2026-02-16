#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "functionalities/visuals/see-trough/esp.h"
#include "memory/data.h"
#include "memory/entity/weapon.h"

// Snapshot of all game data needed by the render thread.
// Copied under mutex so the render thread never touches /proc.
struct GameSnapshot {
  bool connected = false;

  // ESP data
  std::vector<esp::EspPlayerData> players;
  std::vector<esp::GrenadeProximityData> grenadeProximity;
  memory::Mat4 viewMatrix{};
  memory::IVec2 windowSize{};
  uint8_t localTeam = 0;
  std::string localWeapon;
  bool localIsScoped = false;
  int32_t localMvpCount = 0;
  memory::Vec2 localViewAngles{};
  float localFov = 90.0f;
  memory::BombData bomb;

  // Grenade prediction helper
  bool hasLocalPlayer = false;
  memory::Vec3 localEyePos{};
  memory::Vec3 localVelocity{};
  memory::Vec2 localViewAnglesRaw{};
  memory::Weapon localWeaponEnum{};
  bool localPlayerValid = false;
};

class GameDataThread {
public:
  GameDataThread();
  ~GameDataThread();

  // Start/stop the background thread
  void start();
  void stop();

  // Get a copy of the latest snapshot (thread-safe)
  GameSnapshot getSnapshot() const;

  // Check if running
  bool isRunning() const { return running_.load(); }

private:
  void run();

  std::atomic<bool> running_{false};
  std::thread thread_;
  mutable std::mutex mutex_;
  GameSnapshot snapshot_;
};
