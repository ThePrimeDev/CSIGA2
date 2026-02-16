#pragma once

#include <cstdint>
#include <array>
#include <string_view>

namespace memory {
namespace cs2 {

// Process name
constexpr const char* PROCESS_NAME = "cs2";

// Library names
constexpr const char* CLIENT_LIB = "libclient.so";
constexpr const char* ENGINE_LIB = "libengine2.so";
constexpr const char* TIER0_LIB = "libtier0.so";
constexpr const char* INPUT_LIB = "libinputsystem.so";
constexpr const char* SDL_LIB = "libSDL3.so.0";
constexpr const char* SCHEMA_LIB = "libschemasystem.so";

constexpr std::array<const char*, 6> LIBS = {
    CLIENT_LIB, ENGINE_LIB, TIER0_LIB, INPUT_LIB, SDL_LIB, SCHEMA_LIB
};

// Team constants
constexpr uint8_t TEAM_T = 2;
constexpr uint8_t TEAM_CT = 3;

// Misc
constexpr const char* WEAPON_UNKNOWN = "unknown";
constexpr uint32_t DEFAULT_FOV = 90;

// Sound ESP defaults
constexpr float SOUND_ESP_FOOTSTEP_DIAMETER_DEFAULT = 2000.0f;
constexpr float SOUND_ESP_GUNSHOT_DIAMETER_DEFAULT = 8000.0f;
constexpr float SOUND_ESP_WEAPON_DIAMETER_DEFAULT = 3000.0f;

// Entity class names (from RTTI)
namespace entity_class {
    constexpr const char* PLAYER_CONTROLLER = "19CCSPlayerController";
    constexpr const char* PLANTED_C4 = "11C_PlantedC4";
    constexpr const char* INFERNO = "9C_Inferno";
    constexpr const char* SMOKE = "24C_SmokeGrenadeProjectile";
    constexpr const char* MOLOTOV = "19C_MolotovProjectile";
    constexpr const char* FLASHBANG = "21C_FlashbangProjectile";
    constexpr const char* HE_GRENADE = "21C_HEGrenadeProjectile";
    constexpr const char* DECOY = "17C_DecoyProjectile";
} // namespace entity_class

} // namespace cs2

namespace elf {

// ELF header offsets
constexpr uint64_t PROGRAM_HEADER_OFFSET = 0x20;
constexpr uint64_t PROGRAM_HEADER_ENTRY_SIZE = 0x36;
constexpr uint64_t PROGRAM_HEADER_NUM_ENTRIES = 0x38;

constexpr uint64_t SECTION_HEADER_OFFSET = 0x28;
constexpr uint64_t SECTION_HEADER_ENTRY_SIZE = 0x3A;
constexpr uint64_t SECTION_HEADER_NUM_ENTRIES = 0x3C;

constexpr uint64_t DYNAMIC_SECTION_PHT_TYPE = 0x02;

} // namespace elf
} // namespace memory
