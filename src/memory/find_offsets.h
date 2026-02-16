#pragma once

#include "offsets.h"
#include "process.h"
#include <optional>

namespace memory {

// Find all offsets using pattern scanning and schema parsing
std::optional<Offsets> findOffsets(const Process& process);

} // namespace memory
