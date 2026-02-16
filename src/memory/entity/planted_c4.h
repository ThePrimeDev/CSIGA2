#pragma once

#include "../math.h"
#include "../process.h"
#include "../offsets.h"

namespace memory {

class PlantedC4 {
public:
    explicit PlantedC4(uint64_t handle) : handle_(handle) {}

    bool isRelevant(const Process& process, const Offsets& offsets) const;
    bool hasExploded(const Process& process, const Offsets& offsets) const;
    bool isDefused(const Process& process, const Offsets& offsets) const;
    bool isPlanted(const Process& process, const Offsets& offsets) const;
    bool isBeingDefused(const Process& process, const Offsets& offsets) const;
    float timeToExplosion(const Process& process, const Offsets& offsets, float currentTime) const;
    Vec3 position(const Process& process, const Offsets& offsets) const;
    float timeToDefuse(const Process& process, const Offsets& offsets, float currentTime) const;
    int32_t bombSite(const Process& process, const Offsets& offsets) const;

    uint64_t getHandle() const { return handle_; }

private:
    uint64_t handle_;
};

} // namespace memory
