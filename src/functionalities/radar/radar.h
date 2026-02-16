#pragma once

#include <imgui.h>
#include <vector>
#include "functionalities/visuals/see-trough/esp.h" // For EspPlayerData

namespace functionalities {

void DrawRadar(bool menu_visible, 
               const std::vector<esp::EspPlayerData>& players,
               const memory::Vec3& local_pos,
               float local_yaw,
               uint8_t local_team);

} // namespace functionalities
