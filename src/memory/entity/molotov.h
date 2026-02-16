#pragma once

#include "../math.h"
#include "../process.h"
#include "../offsets.h"

namespace memory {

struct MolotovInfo {
    uint64_t entity;
    Vec3 position;
    bool is_incendiary;
};

class Molotov {
public:
    explicit Molotov(uint64_t controller) : controller_(controller) {}

    MolotovInfo info(const Process& process, const Offsets& offsets) const;
    bool isIncendiary(const Process& process, const Offsets& offsets) const;

    uint64_t getController() const { return controller_; }

private:
    uint64_t controller_;
};

} // namespace memory
