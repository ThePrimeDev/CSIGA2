#pragma once

#include <string>

namespace config {

bool SaveConfig(const std::string &path);
bool LoadConfig(const std::string &path);

}  // namespace config
