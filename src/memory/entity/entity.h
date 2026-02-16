#pragma once

#include "../math.h"
#include "player.h"
#include "weapon.h"
#include "smoke.h"
#include "molotov.h"
#include "inferno.h"
#include "planted_c4.h"
#include <variant>
#include <vector>
#include <optional>

namespace memory {

// Entity variant type
struct WeaponEntity {
    Weapon weapon;
    uint64_t entity;
};

using Entity = std::variant<
    WeaponEntity,
    Inferno,
    Smoke,
    Molotov,
    uint64_t  // For Flashbang, HeGrenade, Decoy (just entity address)
>;

// Grenade info for simple grenade types
struct GrenadeInfo {
    uint64_t entity;
    Vec3 position;
    const char* name;
};

// Entity info variants for data transfer
struct WeaponInfo {
    Weapon weapon;
    Vec3 position;
};

using EntityInfo = std::variant<
    WeaponInfo,
    InfernoInfo,
    SmokeInfo,
    MolotovInfo,
    GrenadeInfo  // For Flashbang, HeGrenade, Decoy
>;

// Entity caching utilities
class EntityCache {
public:
    void clear();

    void cacheEntities(const Process& process, const Offsets& offsets);

    const std::vector<Player>& getPlayers() const { return players_; }
    const std::vector<Entity>& getEntities() const { return entities_; }
    const std::optional<PlantedC4>& getPlantedC4() const { return plantedC4_; }
    const Weapon& getCurrentWeapon() const { return currentWeapon_; }
    uint64_t getLocalPawnIndex() const { return localPawnIndex_; }

private:
    void processEntityBucket(
        uint64_t bucketIndex,
        uint64_t bucketPtr,
        const Player& localPlayer,
        const Process& process,
        const Offsets& offsets
    );

    std::vector<Player> players_;
    std::vector<Entity> entities_;
    std::optional<PlantedC4> plantedC4_;
    Weapon currentWeapon_ = Weapon::Unknown;
    uint64_t localPawnIndex_ = 0;
};

} // namespace memory
