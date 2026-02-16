#pragma once

#include <cstdint>
#include <string>

namespace memory {

// Forward declarations
class Process;
struct Offsets;

enum class Weapon {
    Unknown = 0,

    Knife,

    // Pistols
    Cz75A,
    Deagle,
    DualBerettas,
    FiveSeven,
    Glock,
    P2000,
    P250,
    Revolver,
    Tec9,
    Usp,

    // SMGs
    Bizon,
    Mac10,
    Mp5Sd,
    Mp7,
    Mp9,
    P90,
    Ump45,

    // LMGs
    M249,
    Negev,

    // Shotguns
    Mag7,
    Nova,
    Sawedoff,
    Xm1014,

    // Rifles
    Ak47,
    Aug,
    Famas,
    Galilar,
    M4A4,
    M4A1,
    Sg556,

    // Snipers
    Awp,
    G3SG1,
    Scar20,
    Ssg08,

    // Utility
    Taser,

    // Grenades
    Flashbang,
    HeGrenade,
    Smoke,
    Molotov,
    Decoy,
    Incendiary,

    // Bomb
    C4
};

// Get weapon from entity handle
Weapon weaponFromHandle(uint64_t handle, const Process& process, const Offsets& offsets);

// Get weapon from item definition index
Weapon weaponFromIndex(uint16_t index);

// Get weapon display name
const char* weaponToString(Weapon weapon);

// Get weapon icon character (font-specific)
const char* weaponToIcon(Weapon weapon);

} // namespace memory
