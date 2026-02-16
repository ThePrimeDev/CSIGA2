#pragma once

#include "../math.h"
#include "../bones.h"
#include "../process.h"
#include "../offsets.h"
#include "weapon.h"
#include "weapon_class.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace memory {

class Player {
public:
    Player() : controller_(0), pawn_(0) {}
    Player(uint64_t controller, uint64_t pawn) : controller_(controller), pawn_(pawn) {}

    // Factory methods
    static Player entity(uint64_t entity);
    static Player pawn(uint64_t pawn);
    static std::optional<Player> localPlayer(const Process& process, const Offsets& offsets);
    static std::optional<Player> fromController(uint64_t controller, const Process& process, const Offsets& offsets);
    static std::optional<Player> index(const Process& process, const Offsets& offsets, uint64_t index);

    // Entity lookup
    static std::optional<uint64_t> getClientEntity(const Process& process, const Offsets& offsets, uint64_t index);
    static std::optional<uint64_t> getEntity(const Process& process, const Offsets& offsets, int32_t handle);

    // Player properties
    int32_t health(const Process& process, const Offsets& offsets) const;
    int32_t armor(const Process& process, const Offsets& offsets) const;
    uint8_t team(const Process& process, const Offsets& offsets) const;
    uint8_t lifeState(const Process& process, const Offsets& offsets) const;
    uint64_t steamId(const Process& process, const Offsets& offsets) const;
    std::string name(const Process& process, const Offsets& offsets) const;
    int32_t ping(const Process& process, const Offsets& offsets) const;
    int32_t mvpCount(const Process& process, const Offsets& offsets) const;

    // Weapon
    std::string weaponName(const Process& process, const Offsets& offsets) const;
    WeaponClass weaponClass(const Process& process, const Offsets& offsets) const;
    Weapon weapon(const Process& process, const Offsets& offsets) const;
    std::vector<Weapon> allWeapons(const Process& process, const Offsets& offsets) const;

    // Position and transform
    Vec3 position(const Process& process, const Offsets& offsets) const;
    Vec3 eyePosition(const Process& process, const Offsets& offsets) const;
    Vec3 bonePosition(const Process& process, const Offsets& offsets, uint64_t boneIndex) const;
    std::unordered_map<Bones, Vec3> allBones(const Process& process, const Offsets& offsets) const;
    Vec2 viewAngles(const Process& process, const Offsets& offsets) const;
    Vec2 aimPunch(const Process& process, const Offsets& offsets) const;
    Vec3 velocity(const Process& process, const Offsets& offsets) const;
    float rotation(const Process& process, const Offsets& offsets) const;

    // State queries
    bool isValid(const Process& process, const Offsets& offsets) const;
    bool isDormant(const Process& process, const Offsets& offsets) const;
    bool isFlashed(const Process& process, const Offsets& offsets) const;
    bool isScoped(const Process& process, const Offsets& offsets) const;
    bool deathmatchImmunity(const Process& process, const Offsets& offsets) const;

    // Equipment
    bool hasDefuser(const Process& process, const Offsets& offsets) const;
    bool hasHelmet(const Process& process, const Offsets& offsets) const;
    bool hasBomb(const Process& process, const Offsets& offsets) const;

    // Combat
    int32_t shotsFired(const Process& process, const Offsets& offsets) const;
    float fovMultiplier(const Process& process, const Offsets& offsets) const;
    int64_t spottedMask(const Process& process, const Offsets& offsets) const;
    std::optional<Player> crosshairEntity(const Process& process, const Offsets& offsets) const;
    std::optional<Player> spectatorTarget(const Process& process, const Offsets& offsets) const;
    int32_t color(const Process& process, const Offsets& offsets) const;

    // Actions (writes to game memory)
    void noFlash(const Process& process, const Offsets& offsets, float flashAlpha) const;
    void setFov(const Process& process, const Offsets& offsets, uint32_t value) const;
    void setViewAngles(const Process& process, const Offsets& offsets, const Vec2& angles) const;

    // Access
    uint64_t getController() const { return controller_; }
    uint64_t getPawn() const { return pawn_; }

    bool operator==(const Player& other) const {
        return pawn_ == other.pawn_ && controller_ == other.controller_;
    }

private:
    uint64_t gameSceneNode(const Process& process, const Offsets& offsets) const;
    bool isInAir(const Process& process, const Offsets& offsets) const;

    uint64_t controller_;
    uint64_t pawn_;
};

} // namespace memory
