#include "entity.h"
#include "../constants.h"
#include <cstring>

namespace memory {

// Smoke implementation
SmokeInfo Smoke::info(const Process& process, const Offsets& offsets) const {
    SmokeInfo info;
    info.entity = controller_;
    info.position = Player::entity(controller_).position(process, offsets);
    return info;
}

void Smoke::disable(const Process& process, const Offsets& offsets) const {
    uint8_t disabled = process.read<uint8_t>(controller_ + offsets.smoke.did_smoke_effect);
    if (disabled == 0) {
        process.write<uint8_t>(controller_ + offsets.smoke.did_smoke_effect, 1);
    }
}

void Smoke::color(const Process& process, const Offsets& offsets, float r, float g, float b) const {
    uint64_t offset = controller_ + offsets.smoke.smoke_color;
    float currentColor[3];
    currentColor[0] = process.read<float>(offset);
    currentColor[1] = process.read<float>(offset + 4);
    currentColor[2] = process.read<float>(offset + 8);

    float wantedColor[3] = {r * 255.0f, g * 255.0f, b * 255.0f};
    if (currentColor[0] != wantedColor[0] ||
        currentColor[1] != wantedColor[1] ||
        currentColor[2] != wantedColor[2]) {
        process.write<float>(offset, wantedColor[0]);
        process.write<float>(offset + 4, wantedColor[1]);
        process.write<float>(offset + 8, wantedColor[2]);
    }
}

// Molotov implementation
MolotovInfo Molotov::info(const Process& process, const Offsets& offsets) const {
    MolotovInfo info;
    info.entity = controller_;
    info.position = Player::entity(controller_).position(process, offsets);
    info.is_incendiary = isIncendiary(process, offsets);
    return info;
}

bool Molotov::isIncendiary(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(controller_ + offsets.molotov.is_incendiary) != 0;
}

// Inferno implementation
InfernoInfo Inferno::info(const Process& process, const Offsets& offsets) const {
    InfernoInfo info;
    info.entity = controller_;
    info.position = Player::entity(controller_).position(process, offsets);
    info.hull = hull(process, offsets);
    return info;
}

bool Inferno::isBurning(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(controller_ + offsets.inferno.is_burning) != 0;
}

std::vector<Vec3> Inferno::hull(const Process& process, const Offsets& offsets) const {
    std::vector<Vec3> result;

    if (!isBurning(process, offsets)) return result;

    int32_t count = process.read<int32_t>(controller_ + offsets.inferno.fire_count);
    if (count < 0 || count > 64) return result;

    result = process.readTypedVec<Vec3>(
        controller_ + offsets.inferno.fire_positions,
        sizeof(Vec3),
        static_cast<size_t>(count)
    );

    return result;
}

// PlantedC4 implementation
bool PlantedC4::isRelevant(const Process& process, const Offsets& offsets) const {
    return !hasExploded(process, offsets) && !isDefused(process, offsets);
}

bool PlantedC4::hasExploded(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(handle_ + offsets.planted_c4.has_exploded) != 0;
}

bool PlantedC4::isDefused(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(handle_ + offsets.planted_c4.is_defused) != 0;
}

bool PlantedC4::isPlanted(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(handle_ + offsets.planted_c4.is_ticking) != 0;
}

bool PlantedC4::isBeingDefused(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(handle_ + offsets.planted_c4.being_defused) != 0;
}

float PlantedC4::timeToExplosion(const Process& process, const Offsets& offsets, float currentTime) const {
    float blowTime = process.read<float>(handle_ + offsets.planted_c4.blow_time);
    return blowTime - currentTime;
}

Vec3 PlantedC4::position(const Process& process, const Offsets& offsets) const {
    return Player::pawn(handle_).position(process, offsets);
}

float PlantedC4::timeToDefuse(const Process& process, const Offsets& offsets, float currentTime) const {
    float defuseTimeLeft = process.read<float>(handle_ + offsets.planted_c4.defuse_time_left);
    return defuseTimeLeft - currentTime;
}

int32_t PlantedC4::bombSite(const Process& process, const Offsets& offsets) const {
    return process.read<int32_t>(handle_ + offsets.planted_c4.bomb_site);
}

// EntityCache implementation
void EntityCache::clear() {
    players_.clear();
    entities_.clear();
    plantedC4_ = std::nullopt;
    currentWeapon_ = Weapon::Unknown;
    localPawnIndex_ = 0;
}

void EntityCache::cacheEntities(const Process& process, const Offsets& offsets) {
    clear();

    auto localPlayerOpt = Player::localPlayer(process, offsets);
    if (!localPlayerOpt.has_value()) return;

    const Player& localPlayer = localPlayerOpt.value();
    currentWeapon_ = localPlayer.weapon(process, offsets);

    constexpr size_t NUM_BUCKETS = 64;
    auto bucketPointers = process.readVec(offsets.interface_.entity, 0x8 * NUM_BUCKETS);

    for (size_t bucketIndex = 0; bucketIndex < 64; ++bucketIndex) {
        uint64_t bucketPointer;
        std::memcpy(&bucketPointer, &bucketPointers[bucketIndex * 8], sizeof(uint64_t));
        processEntityBucket(bucketIndex, bucketPointer, localPlayer, process, offsets);
    }
}

void EntityCache::processEntityBucket(
    uint64_t bucketIndex,
    uint64_t bucketPtr,
    const Player& localPlayer,
    const Process& process,
    const Offsets& offsets
) {
    if (bucketPtr == 0 || (bucketPtr >> 48) != 0) return;

    constexpr size_t IDENTITIES_PER_BUCKET = 512;
    auto bucket = process.readVec(bucketPtr, IDENTITIES_PER_BUCKET * offsets.entity_identity.size);

    for (size_t indexInBucket = 0; indexInBucket < IDENTITIES_PER_BUCKET; ++indexInBucket) {
        size_t identityOffset = indexInBucket * offsets.entity_identity.size;

        uint64_t entity;
        std::memcpy(&entity, &bucket[identityOffset], sizeof(uint64_t));
        if (entity == 0) continue;

        size_t handleStart = identityOffset + 0x10;
        uint32_t handle;
        std::memcpy(&handle, &bucket[handleStart], sizeof(uint32_t));
        uint32_t handleIndex = handle & 0x7FFF;
        uint32_t entityIndex = static_cast<uint32_t>(bucketIndex * IDENTITIES_PER_BUCKET + indexInBucket);
        if (entityIndex != handleIndex) continue;

        // Get entity class name via RTTI
        uint64_t vtable = process.read<uint64_t>(entity);
        uint64_t rtti = process.read<uint64_t>(vtable - 0x8);
        uint64_t namePtr = process.read<uint64_t>(rtti + 0x8);
        std::string name = process.readStringUncached(namePtr);

        if (name == cs2::entity_class::PLAYER_CONTROLLER) {
            auto playerOpt = Player::fromController(entity, process, offsets);
            if (!playerOpt.has_value()) continue;

            Player player = playerOpt.value();
            if (!player.isValid(process, offsets)) continue;

            if (player == localPlayer) {
                localPawnIndex_ = (static_cast<uint64_t>(handle) & 0x7FFF) - 1;
            } else {
                players_.push_back(player);
            }
        } else if (name == cs2::entity_class::PLANTED_C4) {
            PlantedC4 c4(entity);
            if (c4.isRelevant(process, offsets)) {
                plantedC4_ = c4;
            }
        } else if (name == cs2::entity_class::INFERNO) {
            entities_.push_back(Inferno(entity));
        } else if (name == cs2::entity_class::SMOKE) {
            entities_.push_back(Smoke(entity));
        } else if (name == cs2::entity_class::MOLOTOV) {
            entities_.push_back(Molotov(entity));
        } else if (name == cs2::entity_class::FLASHBANG ||
                   name == cs2::entity_class::HE_GRENADE ||
                   name == cs2::entity_class::DECOY) {
            entities_.push_back(entity);  // Just store entity address
        } else {
            // Check if it's a weapon
            uint64_t entityIdentity = process.read<uint64_t>(entity + 0x10);
            if (entityIdentity == 0) continue;

            uint64_t namePointer = process.read<uint64_t>(entityIdentity + 0x20);
            if (namePointer == 0) continue;

            std::string entityName = process.readStringUncached(namePointer);
            if (entityName.find("weapon_") == 0) {
                // Check if weapon has owner
                int32_t ownerEntity = process.read<int32_t>(entity + offsets.controller.owner_entity);
                if (ownerEntity != -1) continue;

                Weapon weapon = weaponFromHandle(entity, process, offsets);
                entities_.push_back(WeaponEntity{weapon, entity});
            }
        }
    }
}

} // namespace memory
