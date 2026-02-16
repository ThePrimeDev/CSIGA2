#pragma once

#include <cstdint>

namespace memory {

struct LibraryOffsets {
    uint64_t client = 0;
    uint64_t engine = 0;
    uint64_t tier0 = 0;
    uint64_t input = 0;
    uint64_t sdl = 0;
    uint64_t schema = 0;
};

struct InterfaceOffsets {
    uint64_t resource = 0;
    uint64_t entity = 0;
    uint64_t cvar = 0;
    uint64_t input = 0;
};

struct DirectOffsets {
    uint64_t local_player = 0;
    uint64_t button_state = 0;
    uint64_t view_matrix = 0;
    uint64_t sdl_window = 0;
    uint64_t planted_c4 = 0;
    uint64_t global_vars = 0;
};

struct ConvarOffsets {
    uint64_t ffa = 0;
    uint64_t sensitivity = 0;
};

struct PlayerControllerOffsets {
    uint64_t steam_id = 0;      // u64 (m_steamID)
    uint64_t name = 0;          // Pointer -> String (m_iszPlayerName)
    uint64_t sanitized_name = 0;// Pointer -> String (m_sSanitizedPlayerName)
    uint64_t pawn = 0;          // Handle -> Pawn (m_hPawn)
    uint64_t desired_fov = 0;   // u32 (m_iDesiredFOV)
    uint64_t owner_entity = 0;  // i32 (h_pOwnerEntity)
    uint64_t color = 0;         // i32 (m_iCompTeammateColor)
    uint64_t ping = 0;          // u32 (m_iPing)
    uint64_t mvp_count = 0;     // i32 (m_iMVPs)
};

struct PawnOffsets {
    uint64_t health = 0;               // i32 (m_iHealth)
    uint64_t armor = 0;                // i32 (m_ArmorValue)
    uint64_t team = 0;                 // i32 (m_iTeamNum)
    uint64_t life_state = 0;           // i32 (m_lifeState)
    uint64_t weapon = 0;               // Pointer -> WeaponBase (m_pClippingWeapon)
    uint64_t fov_multiplier = 0;       // f32 (m_flFOVSensitivityAdjust)
    uint64_t game_scene_node = 0;      // Pointer -> GameSceneNode (m_pGameSceneNode)
    uint64_t eye_offset = 0;           // Vec3 (m_vecViewOffset)
    uint64_t eye_angles = 0;           // Vec3 (m_angEyeAngles)
    uint64_t velocity = 0;             // Vec3 (m_vecAbsVelocity)
    uint64_t flags = 0;                // i32 (m_fFlags)
    uint64_t aim_punch_cache = 0;      // Vector<Vec3> (m_aimPunchCache)
    uint64_t shots_fired = 0;          // i32 (m_iShotsFired)
    uint64_t view_angles = 0;          // Vec2 (v_angle)
    uint64_t spotted_state = 0;        // SpottedState (m_entitySpottedState)
    uint64_t crosshair_entity = 0;     // EntityIndex (m_iIDEntIndex)
    uint64_t is_scoped = 0;            // bool (m_bIsScoped)
    uint64_t flash_alpha = 0;          // f32 (m_flFlashMaxAlpha)
    uint64_t flash_duration = 0;       // f32 (m_flFlashDuration)
    uint64_t deathmatch_immunity = 0;  // bool (m_bGunGameImmunity)
    uint64_t camera_services = 0;      // Pointer -> CameraServices (m_pCameraServices)
    uint64_t item_services = 0;        // Pointer -> ItemServices (m_pItemServices)
    uint64_t weapon_services = 0;      // Pointer -> WeaponServices (m_pWeaponServices)
    uint64_t observer_services = 0;    // Pointer -> ObserverServices (m_pObserverServices)
};

struct GameSceneNodeOffsets {
    uint64_t dormant = 0;       // bool (m_bDormant)
    uint64_t origin = 0;        // Vec3 (m_vecAbsOrigin)
    uint64_t model_state = 0;   // Pointer -> ModelState (m_modelState)
};

struct SkeletonInstanceOffsets {
    uint64_t skeleton_instance = 0;  // CSkeletonInstance (m_skeletonInstance)
};

struct SmokeOffsets {
    uint64_t did_smoke_effect = 0;  // bool (m_bDidSmokeEffect)
    uint64_t smoke_color = 0;       // Vec3 (m_vSmokeColor)
};

struct MolotovOffsets {
    uint64_t is_incendiary = 0;  // bool (m_bIsIncGrenade)
};

struct InfernoOffsets {
    uint64_t is_burning = 0;      // bool[64] (m_bFireIsBurning)
    uint64_t fire_count = 0;      // i32 (m_fireCount)
    uint64_t fire_positions = 0;  // Vec3[64] (m_firePositions)
};

struct SpottedStateOffsets {
    uint64_t spotted = 0;  // bool (m_bSpotted)
    uint64_t mask = 0;     // i64 (m_bSpottedByMask)
};

struct CameraServicesOffsets {
    uint64_t fov = 0;  // u32 (m_iFOV)
};

struct ItemServicesOffsets {
    uint64_t has_defuser = 0;  // bool (m_bHasDefuser)
    uint64_t has_helmet = 0;   // bool (m_bHasHelmet)
};

struct WeaponServicesOffsets {
    uint64_t weapons = 0;  // Pointer -> Vec<Pointer -> Weapon> (m_hMyWeapons)
};

struct ObserverServicesOffsets {
    uint64_t target = 0;  // Handle -> BaseEntity (m_hObserverTarget)
};

struct WeaponOffsets {
    uint64_t attribute_manager = 0;      // AttributeContainer (m_AttributeManager)
    uint64_t item = 0;                   // EconItemView (m_Item)
    uint64_t item_definition_index = 0;  // u16 (m_iItemDefinitionIndex)
};

struct PlantedC4Offsets {
    uint64_t is_ticking = 0;        // bool (m_bBombTicking)
    uint64_t blow_time = 0;         // f32 (m_flC4Blow)
    uint64_t being_defused = 0;     // bool (m_bBeingDefused)
    uint64_t is_defused = 0;        // bool (m_bBombDefused)
    uint64_t has_exploded = 0;      // bool (m_bHasExploded)
    uint64_t defuse_time_left = 0;  // f32 (m_flDefuseCountDown)
    uint64_t bomb_site = 0;         // i32 (m_nBombSite)
};

struct GlowOffsets {
    uint64_t property = 0;       // C_BaseModelEntity::m_Glow (CGlowProperty)
    uint64_t enabled = 0;        // CGlowProperty::m_bGlowEnabled
    uint64_t through_walls = 0;  // CGlowProperty::m_bGlowThroughWalls
};

struct EntityIdentityOffsets {
    int32_t size = 0;
};

struct Offsets {
    LibraryOffsets library;
    InterfaceOffsets interface_;
    DirectOffsets direct;
    ConvarOffsets convar;
    PlayerControllerOffsets controller;
    PawnOffsets pawn;
    GameSceneNodeOffsets game_scene_node;
    SkeletonInstanceOffsets skeleton;
    SmokeOffsets smoke;
    MolotovOffsets molotov;
    InfernoOffsets inferno;
    SpottedStateOffsets spotted_state;
    CameraServicesOffsets camera_services;
    ItemServicesOffsets item_services;
    WeaponServicesOffsets weapon_services;
    ObserverServicesOffsets observer_services;
    WeaponOffsets weapon;
    PlantedC4Offsets planted_c4;
    GlowOffsets glow;
    EntityIdentityOffsets entity_identity;
};

} // namespace memory
