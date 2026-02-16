#include "player.h"
#include "../constants.h"

namespace memory {

// Factory methods
Player Player::entity(uint64_t entity) {
    return Player(0, entity);
}

Player Player::pawn(uint64_t pawn) {
    return Player(0, pawn);
}

std::optional<Player> Player::localPlayer(const Process& process, const Offsets& offsets) {
    uint64_t controller = process.read<uint64_t>(offsets.direct.local_player);
    if (controller == 0) return std::nullopt;

    int32_t pawnHandle = process.read<int32_t>(controller + offsets.controller.pawn);
    if (pawnHandle == -1) return std::nullopt;

    auto pawnOpt = getEntity(process, offsets, pawnHandle);
    if (!pawnOpt.has_value()) return std::nullopt;

    return Player(controller, pawnOpt.value());
}

std::optional<Player> Player::fromController(uint64_t controller, const Process& process, const Offsets& offsets) {
    int32_t pawnHandle = process.read<int32_t>(controller + offsets.controller.pawn);
    if (pawnHandle == -1) return std::nullopt;

    auto pawnOpt = getEntity(process, offsets, pawnHandle);
    if (!pawnOpt.has_value()) return std::nullopt;

    return Player(controller, pawnOpt.value());
}

std::optional<Player> Player::index(const Process& process, const Offsets& offsets, uint64_t idx) {
    auto controllerOpt = getClientEntity(process, offsets, idx);
    if (!controllerOpt.has_value()) return std::nullopt;

    int32_t pawnHandle = process.read<int32_t>(controllerOpt.value() + offsets.controller.pawn);
    if (pawnHandle == -1) return std::nullopt;

    auto pawnOpt = getEntity(process, offsets, pawnHandle);
    if (!pawnOpt.has_value()) return std::nullopt;

    return Player(controllerOpt.value(), pawnOpt.value());
}

std::optional<uint64_t> Player::getClientEntity(const Process& process, const Offsets& offsets, uint64_t idx) {
    uint64_t bucketIndex = idx >> 9;
    uint64_t indexInBucket = idx & 0x1FF;

    uint64_t bucketPtr = process.read<uint64_t>(offsets.interface_.entity + 0x08 * bucketIndex);
    if (bucketPtr == 0) return std::nullopt;

    uint64_t entity = process.read<uint64_t>(bucketPtr + offsets.entity_identity.size * indexInBucket);
    if (entity == 0) return std::nullopt;

    return entity;
}

std::optional<uint64_t> Player::getEntity(const Process& process, const Offsets& offsets, int32_t handle) {
    uint64_t idx = static_cast<uint64_t>(handle) & 0x7FFF;
    uint64_t bucketIndex = idx >> 9;
    uint64_t indexInBucket = idx & 0x1FF;

    uint64_t bucketPtr = process.read<uint64_t>(offsets.interface_.entity + 8 * bucketIndex);
    if (bucketPtr == 0) return std::nullopt;

    uint64_t entity = process.read<uint64_t>(bucketPtr + offsets.entity_identity.size * indexInBucket);
    if (entity == 0) return std::nullopt;

    return entity;
}

// Properties
int32_t Player::health(const Process& process, const Offsets& offsets) const {
    int32_t hp = process.read<int32_t>(pawn_ + offsets.pawn.health);
    if (hp < 0 || hp > 100) return 0;
    return hp;
}

int32_t Player::armor(const Process& process, const Offsets& offsets) const {
    return process.read<int32_t>(pawn_ + offsets.pawn.armor);
}

uint8_t Player::team(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(pawn_ + offsets.pawn.team);
}

uint8_t Player::lifeState(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(pawn_ + offsets.pawn.life_state);
}

uint64_t Player::steamId(const Process& process, const Offsets& offsets) const {
    return process.read<uint64_t>(controller_ + offsets.controller.steam_id);
}

std::string Player::name(const Process& process, const Offsets& offsets) const {
    std::string playerName = process.readStringUncached(controller_ + offsets.controller.name);
    if (!playerName.empty()) return playerName;

    // Fallback to sanitized name if available
    if (offsets.controller.sanitized_name != 0) {
        uint64_t sanitizedNamePtr = process.read<uint64_t>(controller_ + offsets.controller.sanitized_name);
        if (sanitizedNamePtr != 0) {
             return process.readStringUncached(sanitizedNamePtr);
        }
    }
    return "";
}

int32_t Player::ping(const Process& process, const Offsets& offsets) const {
    return process.read<int32_t>(controller_ + offsets.controller.ping);
}

int32_t Player::mvpCount(const Process& process, const Offsets& offsets) const {
    if (offsets.controller.mvp_count == 0) return 0;
    return process.read<int32_t>(controller_ + offsets.controller.mvp_count);
}

std::string Player::weaponName(const Process& process, const Offsets& offsets) const {
    uint64_t weaponEntityInstance = process.read<uint64_t>(pawn_ + offsets.pawn.weapon);
    if (weaponEntityInstance == 0) return cs2::WEAPON_UNKNOWN;

    uint64_t weaponEntityIdentity = process.read<uint64_t>(weaponEntityInstance + 0x10);
    if (weaponEntityIdentity == 0) return cs2::WEAPON_UNKNOWN;

    uint64_t weaponNamePointer = process.read<uint64_t>(weaponEntityIdentity + 0x20);
    if (weaponNamePointer == 0) return cs2::WEAPON_UNKNOWN;

    std::string name = process.readStringUncached(weaponNamePointer);
    // Remove "weapon_" prefix
    const std::string prefix = "weapon_";
    if (name.substr(0, prefix.size()) == prefix) {
        return name.substr(prefix.size());
    }
    return name;
}

WeaponClass Player::weaponClass(const Process& process, const Offsets& offsets) const {
    return weaponClassFromString(weaponName(process, offsets));
}

Weapon Player::weapon(const Process& process, const Offsets& offsets) const {
    uint64_t currentWeapon = process.read<uint64_t>(pawn_ + offsets.pawn.weapon);
    return weaponFromHandle(currentWeapon, process, offsets);
}

std::vector<Weapon> Player::allWeapons(const Process& process, const Offsets& offsets) const {
    std::vector<Weapon> weapons;

    uint64_t weaponServices = process.read<uint64_t>(pawn_ + offsets.pawn.weapon_services);
    if (weaponServices == 0) return weapons;

    int32_t length = process.read<int32_t>(weaponServices + offsets.weapon_services.weapons);
    uint64_t weaponList = process.read<uint64_t>(weaponServices + offsets.weapon_services.weapons + 0x08);

    for (int32_t i = 0; i < length; ++i) {
        uint64_t weaponIndex = static_cast<uint64_t>(process.read<int32_t>(weaponList + 0x04 * i)) & 0xFFF;
        auto weaponEntityOpt = getClientEntity(process, offsets, weaponIndex);
        if (!weaponEntityOpt.has_value() || weaponEntityOpt.value() == 0) continue;

        weapons.push_back(weaponFromHandle(weaponEntityOpt.value(), process, offsets));
    }

    return weapons;
}

uint64_t Player::gameSceneNode(const Process& process, const Offsets& offsets) const {
    return process.read<uint64_t>(pawn_ + offsets.pawn.game_scene_node);
}

Vec3 Player::position(const Process& process, const Offsets& offsets) const {
    uint64_t gsNode = gameSceneNode(process, offsets);
    return process.read<Vec3>(gsNode + offsets.game_scene_node.origin);
}

Vec3 Player::eyePosition(const Process& process, const Offsets& offsets) const {
    Vec3 pos = position(process, offsets);
    Vec3 eyeOffset = process.read<Vec3>(pawn_ + offsets.pawn.eye_offset);
    return pos + eyeOffset;
}

Vec3 Player::bonePosition(const Process& process, const Offsets& offsets, uint64_t boneIndex) const {
    uint64_t gsNode = gameSceneNode(process, offsets);
    uint64_t boneData = process.read<uint64_t>(
        gsNode + offsets.game_scene_node.model_state + offsets.skeleton.skeleton_instance
    );

    if (boneData == 0) return Vec3::ZERO;

    return process.read<Vec3>(boneData + (boneIndex * 32));
}

std::unordered_map<Bones, Vec3> Player::allBones(const Process& process, const Offsets& offsets) const {
    std::unordered_map<Bones, Vec3> bones;

    uint64_t gsNode = gameSceneNode(process, offsets);
    uint64_t boneData = process.read<uint64_t>(
        gsNode + offsets.game_scene_node.model_state + offsets.skeleton.skeleton_instance
    );

    if (boneData == 0) return bones;

    // Read all bone data at once
    struct BoneData { float pos[3]; float pad[5]; };  // 32 bytes per bone
    std::array<BoneData, 32> bonesData;
    bonesData = process.readOrZeroed<std::array<BoneData, 32>>(boneData);

    for (Bones bone : ALL_BONES) {
        size_t idx = static_cast<size_t>(bone);
        if (idx < 32) {
            bones[bone] = Vec3(bonesData[idx].pos[0], bonesData[idx].pos[1], bonesData[idx].pos[2]);
        }
    }

    return bones;
}

Vec2 Player::viewAngles(const Process& process, const Offsets& offsets) const {
    return process.read<Vec2>(pawn_ + offsets.pawn.view_angles);
}

Vec2 Player::aimPunch(const Process& process, const Offsets& offsets) const {
    uint64_t length = process.read<uint64_t>(pawn_ + offsets.pawn.aim_punch_cache);
    if (length < 1) return Vec2::ZERO;

    uint64_t dataAddress = process.read<uint64_t>(pawn_ + offsets.pawn.aim_punch_cache + 0x08);
    if (dataAddress > UINT64_MAX - 50000) return Vec2::ZERO;

    return process.read<Vec2>(dataAddress + (length - 1) * 12);
}

Vec3 Player::velocity(const Process& process, const Offsets& offsets) const {
    return process.read<Vec3>(pawn_ + offsets.pawn.velocity);
}

float Player::rotation(const Process& process, const Offsets& offsets) const {
    return process.read<float>(pawn_ + offsets.pawn.eye_angles + 0x04);
}

bool Player::isDormant(const Process& process, const Offsets& offsets) const {
    uint64_t gsNode = gameSceneNode(process, offsets);
    return process.read<uint8_t>(gsNode + offsets.game_scene_node.dormant) != 0;
}

bool Player::isValid(const Process& process, const Offsets& offsets) const {
    if (isDormant(process, offsets)) return false;
    if (health(process, offsets) <= 0) return false;
    if (lifeState(process, offsets) != 0) return false;
    if (deathmatchImmunity(process, offsets)) return false;
    return true;
}

bool Player::isFlashed(const Process& process, const Offsets& offsets) const {
    return process.read<float>(pawn_ + offsets.pawn.flash_duration) > 0.2f;
}

bool Player::isScoped(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(pawn_ + offsets.pawn.is_scoped) != 0;
}

bool Player::deathmatchImmunity(const Process& process, const Offsets& offsets) const {
    return process.read<uint8_t>(pawn_ + offsets.pawn.deathmatch_immunity) != 0;
}

bool Player::hasDefuser(const Process& process, const Offsets& offsets) const {
    uint64_t itemServices = process.read<uint64_t>(pawn_ + offsets.pawn.item_services);
    if (itemServices == 0) return false;
    return process.read<uint8_t>(itemServices + offsets.item_services.has_defuser) != 0;
}

bool Player::hasHelmet(const Process& process, const Offsets& offsets) const {
    uint64_t itemServices = process.read<uint64_t>(pawn_ + offsets.pawn.item_services);
    if (itemServices == 0) return false;
    return process.read<uint8_t>(itemServices + offsets.item_services.has_helmet) != 0;
}

bool Player::hasBomb(const Process& process, const Offsets& offsets) const {
    auto weapons = allWeapons(process, offsets);
    for (const auto& w : weapons) {
        if (w == Weapon::C4) return true;
    }
    return false;
}

int32_t Player::shotsFired(const Process& process, const Offsets& offsets) const {
    return process.read<int32_t>(pawn_ + offsets.pawn.shots_fired);
}

float Player::fovMultiplier(const Process& process, const Offsets& offsets) const {
    return process.read<float>(pawn_ + offsets.pawn.fov_multiplier);
}

int64_t Player::spottedMask(const Process& process, const Offsets& offsets) const {
    return process.read<int64_t>(pawn_ + offsets.pawn.spotted_state + offsets.spotted_state.mask);
}

std::optional<Player> Player::crosshairEntity(const Process& process, const Offsets& offsets) const {
    int32_t idx = process.read<int32_t>(pawn_ + offsets.pawn.crosshair_entity);
    if (idx == -1) return std::nullopt;

    auto entityOpt = getClientEntity(process, offsets, static_cast<uint64_t>(idx));
    if (!entityOpt.has_value()) return std::nullopt;

    Player player(0, entityOpt.value());
    if (!player.isValid(process, offsets)) return std::nullopt;

    return player;
}

std::optional<Player> Player::spectatorTarget(const Process& process, const Offsets& offsets) const {
    uint64_t observerServices = process.read<uint64_t>(pawn_ + offsets.pawn.observer_services);
    if (observerServices == 0) return std::nullopt;

    int32_t target = process.read<int32_t>(observerServices + offsets.observer_services.target);
    if (target == -1) return std::nullopt;

    auto pawnOpt = getEntity(process, offsets, target);
    if (!pawnOpt.has_value()) return std::nullopt;

    return Player::pawn(pawnOpt.value());
}

int32_t Player::color(const Process& process, const Offsets& offsets) const {
    return process.read<int32_t>(controller_ + offsets.controller.color);
}

bool Player::isInAir(const Process& process, const Offsets& offsets) const {
    int32_t flags = process.read<int32_t>(pawn_ + offsets.pawn.flags);
    return (flags & 1) == 0;  // FL_ONGROUND = (1 << 0)
}

void Player::noFlash(const Process& process, const Offsets& offsets, float flashAlpha) const {
    float clampedAlpha = std::max(0.0f, std::min(255.0f, flashAlpha));
    float currentAlpha = process.read<float>(pawn_ + offsets.pawn.flash_alpha);
    if (currentAlpha != clampedAlpha) {
        process.write(pawn_ + offsets.pawn.flash_alpha, clampedAlpha);
    }
}

void Player::setFov(const Process& process, const Offsets& offsets, uint32_t value) const {
    uint64_t cameraService = process.read<uint64_t>(pawn_ + offsets.pawn.camera_services);
    if (cameraService == 0) return;

    uint32_t current = process.read<uint32_t>(cameraService + offsets.camera_services.fov);
    if (current != 0 && current != value) {
        process.write(controller_ + offsets.controller.desired_fov, value);
    }
}

void Player::setViewAngles(const Process& process, const Offsets& offsets, const Vec2& angles) const {
    process.write(pawn_ + offsets.pawn.view_angles, angles);
}

} // namespace memory
