#pragma once

#include "../math.h"
#include "../process.h"
#include "../offsets.h"
#include <vector>

namespace memory {

struct InfernoInfo {
    uint64_t entity;
    Vec3 position;
    std::vector<Vec3> hull;
};

class Inferno {
public:
    explicit Inferno(uint64_t controller) : controller_(controller) {}

    InfernoInfo info(const Process& process, const Offsets& offsets) const;
    bool isBurning(const Process& process, const Offsets& offsets) const;
    std::vector<Vec3> hull(const Process& process, const Offsets& offsets) const;

    uint64_t getController() const { return controller_; }

private:
    uint64_t controller_;
};

} // namespace memory
