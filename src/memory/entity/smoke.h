#pragma once

#include "../math.h"
#include "../process.h"
#include "../offsets.h"

namespace memory {

class Player;

struct SmokeInfo {
    uint64_t entity;
    Vec3 position;
};

class Smoke {
public:
    explicit Smoke(uint64_t controller) : controller_(controller) {}

    SmokeInfo info(const Process& process, const Offsets& offsets) const;

    // Disable smoke effect
    void disable(const Process& process, const Offsets& offsets) const;

    // Change smoke color
    void color(const Process& process, const Offsets& offsets, float r, float g, float b) const;

    uint64_t getController() const { return controller_; }

private:
    uint64_t controller_;
};

} // namespace memory
