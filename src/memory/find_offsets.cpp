#include "find_offsets.h"
#include "constants.h"
#include "schema.h"

namespace memory {

std::optional<Offsets> findOffsets(const Process& process) {
    Offsets offsets;

    // Get library base addresses
    auto clientBase = process.moduleBaseAddress(cs2::CLIENT_LIB);
    if (!clientBase.has_value()) { printf("[OFFSETS] Failed: client lib\n"); return std::nullopt; }
    offsets.library.client = clientBase.value();
    printf("[OFFSETS] Found client: 0x%lx\n", offsets.library.client);

    auto engineBase = process.moduleBaseAddress(cs2::ENGINE_LIB);
    if (!engineBase.has_value()) { printf("[OFFSETS] Failed: engine lib\n"); return std::nullopt; }
    offsets.library.engine = engineBase.value();

    auto tier0Base = process.moduleBaseAddress(cs2::TIER0_LIB);
    if (!tier0Base.has_value()) { printf("[OFFSETS] Failed: tier0 lib\n"); return std::nullopt; }
    offsets.library.tier0 = tier0Base.value();

    auto inputBase = process.moduleBaseAddress(cs2::INPUT_LIB);
    if (!inputBase.has_value()) { printf("[OFFSETS] Failed: input lib\n"); return std::nullopt; }
    offsets.library.input = inputBase.value();

    auto sdlBase = process.moduleBaseAddress(cs2::SDL_LIB);
    if (!sdlBase.has_value()) { printf("[OFFSETS] Failed: sdl lib\n"); return std::nullopt; }
    offsets.library.sdl = sdlBase.value();

    auto schemaBase = process.moduleBaseAddress(cs2::SCHEMA_LIB);
    if (!schemaBase.has_value()) { printf("[OFFSETS] Failed: schema lib\n"); return std::nullopt; }
    offsets.library.schema = schemaBase.value();
    printf("[OFFSETS] All libs found\n");

    // Get interface offsets
    auto resourceOffset = process.getInterfaceOffset(offsets.library.engine, "GameResourceServiceClientV0");
    if (!resourceOffset.has_value()) { printf("[OFFSETS] Failed: resource interface\n"); return std::nullopt; }
    offsets.interface_.resource = resourceOffset.value();

    offsets.interface_.entity = process.read<uint64_t>(offsets.interface_.resource + 0x50) + 0x10;

    auto cvarOffset = process.getInterfaceOffset(offsets.library.tier0, "VEngineCvar0");
    if (!cvarOffset.has_value()) { printf("[OFFSETS] Failed: cvar interface\n"); return std::nullopt; }
    offsets.interface_.cvar = cvarOffset.value();

    auto inputOffset = process.getInterfaceOffset(offsets.library.input, "InputSystemVersion0");
    if (!inputOffset.has_value()) { printf("[OFFSETS] Failed: input interface\n"); return std::nullopt; }
    offsets.interface_.input = inputOffset.value();
    printf("[OFFSETS] Interfaces found\n");

    // Pattern scans for direct offsets
    auto localPlayer = process.scan("48 83 3D ? ? ? ? 00 0F 95 C0 C3", offsets.library.client);
    if (!localPlayer.has_value()) { printf("[OFFSETS] Failed: localPlayer pattern\n"); return std::nullopt; }
    offsets.direct.local_player = process.getRelativeAddress(localPlayer.value(), 0x03, 0x08);

    offsets.direct.button_state = static_cast<uint64_t>(
        process.read<uint32_t>(process.getInterfaceFunction(offsets.interface_.input, 19) + 0x14)
    );

    auto viewMatrix = process.scan("C6 83 ? ? 00 00 01 4C 8D 05", offsets.library.client);
    if (!viewMatrix.has_value()) { printf("[OFFSETS] Failed: viewMatrix pattern\n"); return std::nullopt; }
    offsets.direct.view_matrix = process.getRelativeAddress(viewMatrix.value() + 0x0A, 0x0, 0x04);

    auto sdlWindowExport = process.getModuleExport(offsets.library.sdl, "SDL_GetKeyboardFocus");
    if (!sdlWindowExport.has_value()) { printf("[OFFSETS] Failed: SDL export\n"); return std::nullopt; }
    uint64_t sdlWindow = process.getRelativeAddress(sdlWindowExport.value(), 0x02, 0x06);
    sdlWindow = process.read<uint64_t>(sdlWindow);
    offsets.direct.sdl_window = process.getRelativeAddress(sdlWindow, 0x03, 0x07);

    auto plantedC4 = process.scan(
        "48 8D 35 ? ? ? ? 66 0F EF C0 C6 05 ? ? ? ? 01 48 8D 3D",
        offsets.library.client
    );
    if (!plantedC4.has_value()) { printf("[OFFSETS] Failed: plantedC4 pattern\n"); return std::nullopt; }
    offsets.direct.planted_c4 = process.getRelativeAddress(plantedC4.value(), 0x03, 0x0E);

    auto globalVars = process.scan(
        "48 8D 05 ? ? ? ? 48 8B 00 8B 50 ? E9",
        offsets.library.client
    );
    if (!globalVars.has_value()) { printf("[OFFSETS] Failed: globalVars pattern\n"); return std::nullopt; }
    offsets.direct.global_vars = process.getRelativeAddress(globalVars.value(), 0x03, 0x07);
    printf("[OFFSETS] Pattern scans done\n");

    // Convar offsets
    auto ffaConvar = process.getConvar(offsets.interface_.cvar, "mp_teammates_are_enemies");
    if (!ffaConvar.has_value()) { printf("[OFFSETS] Failed: ffa convar\n"); return std::nullopt; }
    offsets.convar.ffa = ffaConvar.value();

    auto sensitivityConvar = process.getConvar(offsets.interface_.cvar, "sensitivity");
    if (!sensitivityConvar.has_value()) { printf("[OFFSETS] Failed: sensitivity convar\n"); return std::nullopt; }
    offsets.convar.sensitivity = sensitivityConvar.value();
    printf("[OFFSETS] Convars found\n");

    // Create schema and get remaining offsets
    auto schemaOpt = Schema::create(process, offsets.library.schema);
    if (!schemaOpt.has_value()) { printf("[OFFSETS] Failed: schema creation\n"); return std::nullopt; }
    const Schema& schema = schemaOpt.value();
    printf("[OFFSETS] Schema created\n");

    auto client = schema.getLibrary(cs2::CLIENT_LIB);
    if (!client) { printf("[OFFSETS] Failed: client lib from schema\n"); return std::nullopt; }
    printf("[OFFSETS] Got client library from schema\n");

    // Helper lambda for getting offsets
    auto getOffset = [&](const std::string& className, const std::string& fieldName) -> std::optional<uint64_t> {
        return client->get(className, fieldName);
    };

    // Controller offsets
    auto steamId = getOffset("CBasePlayerController", "m_steamID");
    if (!steamId) return std::nullopt;
    offsets.controller.steam_id = steamId.value();

    auto name = getOffset("CBasePlayerController", "m_iszPlayerName");
    if (!name) return std::nullopt;
    offsets.controller.name = name.value();

    auto pawn = getOffset("CBasePlayerController", "m_hPawn");
    if (!pawn) return std::nullopt;
    offsets.controller.pawn = pawn.value();

    auto desiredFov = getOffset("CBasePlayerController", "m_iDesiredFOV");
    if (!desiredFov) return std::nullopt;
    offsets.controller.desired_fov = desiredFov.value();

    auto ownerEntity = getOffset("C_BaseEntity", "m_hOwnerEntity");
    if (!ownerEntity) return std::nullopt;
    offsets.controller.owner_entity = ownerEntity.value();

    auto color = getOffset("CCSPlayerController", "m_iCompTeammateColor");
    if (!color) return std::nullopt;
    offsets.controller.color = color.value();

    auto ping = getOffset("CCSPlayerController", "m_iPing");
    if (!ping) return std::nullopt;
    offsets.controller.ping = ping.value();

    auto sanitizedName = getOffset("CBasePlayerController", "m_sSanitizedPlayerName");
    // m_sSanitizedPlayerName might not exist in all versions, but let's try.
    // If it fails, we can just leave it as 0.
    if (sanitizedName) {
        offsets.controller.sanitized_name = sanitizedName.value();
    }

    auto glowProperty = getOffset("C_BaseModelEntity", "m_Glow");
    if (!glowProperty) {
        glowProperty = getOffset("C_BaseModelEntity", "m_GlowProperty");
    }
    if (glowProperty) {
        offsets.glow.property = glowProperty.value();
    }

    auto glowEnabled = getOffset("CGlowProperty", "m_bGlowEnabled");
    if (glowEnabled) {
        offsets.glow.enabled = glowEnabled.value();
    }

    auto glowThrough = getOffset("CGlowProperty", "m_bGlowThroughWalls");
    if (glowThrough) {
        offsets.glow.through_walls = glowThrough.value();
    }

    auto mvpCount = getOffset("CCSPlayerController", "m_iMVPs");
    if (mvpCount) {
        offsets.controller.mvp_count = mvpCount.value();
    }

    // Pawn offsets
    auto health = getOffset("C_BaseEntity", "m_iHealth");
    if (!health) return std::nullopt;
    offsets.pawn.health = health.value();

    auto armor = getOffset("C_CSPlayerPawn", "m_ArmorValue");
    if (!armor) return std::nullopt;
    offsets.pawn.armor = armor.value();

    auto team = getOffset("C_BaseEntity", "m_iTeamNum");
    if (!team) return std::nullopt;
    offsets.pawn.team = team.value();

    auto lifeState = getOffset("C_BaseEntity", "m_lifeState");
    if (!lifeState) return std::nullopt;
    offsets.pawn.life_state = lifeState.value();

    auto weapon = getOffset("C_CSPlayerPawn", "m_pClippingWeapon");
    if (!weapon) return std::nullopt;
    offsets.pawn.weapon = weapon.value();

    auto fovMult = getOffset("C_BasePlayerPawn", "m_flFOVSensitivityAdjust");
    if (!fovMult) return std::nullopt;
    offsets.pawn.fov_multiplier = fovMult.value();

    auto gameSceneNode = getOffset("C_BaseEntity", "m_pGameSceneNode");
    if (!gameSceneNode) return std::nullopt;
    offsets.pawn.game_scene_node = gameSceneNode.value();

    auto eyeOffset = getOffset("C_BaseModelEntity", "m_vecViewOffset");
    if (!eyeOffset) return std::nullopt;
    offsets.pawn.eye_offset = eyeOffset.value();

    auto eyeAngles = getOffset("C_CSPlayerPawn", "m_angEyeAngles");
    if (!eyeAngles) return std::nullopt;
    offsets.pawn.eye_angles = eyeAngles.value();

    auto velocity = getOffset("C_BaseEntity", "m_vecAbsVelocity");
    if (!velocity) return std::nullopt;
    offsets.pawn.velocity = velocity.value();

    auto flags = getOffset("C_BaseEntity", "m_fFlags");
    if (!flags) return std::nullopt;
    offsets.pawn.flags = flags.value();

    auto aimPunch = getOffset("C_CSPlayerPawn", "m_aimPunchTickFraction");
    if (!aimPunch) return std::nullopt;
    offsets.pawn.aim_punch_cache = aimPunch.value() + 8;

    auto shotsFired = getOffset("C_CSPlayerPawn", "m_iShotsFired");
    if (!shotsFired) return std::nullopt;
    offsets.pawn.shots_fired = shotsFired.value();

    auto viewAngles = getOffset("C_BasePlayerPawn", "v_angle");
    if (!viewAngles) return std::nullopt;
    offsets.pawn.view_angles = viewAngles.value();

    auto spottedState = getOffset("C_CSPlayerPawn", "m_entitySpottedState");
    if (!spottedState) return std::nullopt;
    offsets.pawn.spotted_state = spottedState.value();

    auto crosshairEntity = getOffset("C_CSPlayerPawn", "m_iIDEntIndex");
    if (!crosshairEntity) return std::nullopt;
    offsets.pawn.crosshair_entity = crosshairEntity.value();

    auto isScoped = getOffset("C_CSPlayerPawn", "m_bIsScoped");
    if (!isScoped) return std::nullopt;
    offsets.pawn.is_scoped = isScoped.value();

    auto flashAlpha = getOffset("C_CSPlayerPawnBase", "m_flFlashMaxAlpha");
    if (!flashAlpha) return std::nullopt;
    offsets.pawn.flash_alpha = flashAlpha.value();

    auto flashDuration = getOffset("C_CSPlayerPawnBase", "m_flFlashDuration");
    if (!flashDuration) return std::nullopt;
    offsets.pawn.flash_duration = flashDuration.value();

    auto dmImmunity = getOffset("C_CSPlayerPawn", "m_bGunGameImmunity");
    if (!dmImmunity) return std::nullopt;
    offsets.pawn.deathmatch_immunity = dmImmunity.value();

    auto cameraServices = getOffset("C_BasePlayerPawn", "m_pCameraServices");
    if (!cameraServices) return std::nullopt;
    offsets.pawn.camera_services = cameraServices.value();

    auto itemServices = getOffset("C_BasePlayerPawn", "m_pItemServices");
    if (!itemServices) return std::nullopt;
    offsets.pawn.item_services = itemServices.value();

    auto weaponServices = getOffset("C_BasePlayerPawn", "m_pWeaponServices");
    if (!weaponServices) return std::nullopt;
    offsets.pawn.weapon_services = weaponServices.value();

    auto observerServices = getOffset("C_BasePlayerPawn", "m_pObserverServices");
    if (!observerServices) return std::nullopt;
    offsets.pawn.observer_services = observerServices.value();

    // GameSceneNode offsets
    auto dormant = getOffset("CGameSceneNode", "m_bDormant");
    if (!dormant) return std::nullopt;
    offsets.game_scene_node.dormant = dormant.value();

    auto origin = getOffset("CGameSceneNode", "m_vecAbsOrigin");
    if (!origin) return std::nullopt;
    offsets.game_scene_node.origin = origin.value();

    auto modelState = getOffset("CSkeletonInstance", "m_modelState");
    if (!modelState) return std::nullopt;
    offsets.game_scene_node.model_state = modelState.value();

    // Skeleton offsets
    auto skeletonInstance = getOffset("CBodyComponentSkeletonInstance", "m_skeletonInstance");
    if (!skeletonInstance) return std::nullopt;
    offsets.skeleton.skeleton_instance = skeletonInstance.value();

    // Smoke offsets
    auto didSmokeEffect = getOffset("C_SmokeGrenadeProjectile", "m_bDidSmokeEffect");
    if (!didSmokeEffect) return std::nullopt;
    offsets.smoke.did_smoke_effect = didSmokeEffect.value();

    auto smokeColor = getOffset("C_SmokeGrenadeProjectile", "m_vSmokeColor");
    if (!smokeColor) return std::nullopt;
    offsets.smoke.smoke_color = smokeColor.value();

    // Molotov offsets
    auto isIncendiary = getOffset("C_MolotovProjectile", "m_bIsIncGrenade");
    if (!isIncendiary) return std::nullopt;
    offsets.molotov.is_incendiary = isIncendiary.value();

    // Inferno offsets
    auto isBurning = getOffset("C_Inferno", "m_bFireIsBurning");
    if (!isBurning) return std::nullopt;
    offsets.inferno.is_burning = isBurning.value();

    auto fireCount = getOffset("C_Inferno", "m_fireCount");
    if (!fireCount) return std::nullopt;
    offsets.inferno.fire_count = fireCount.value();

    auto firePositions = getOffset("C_Inferno", "m_firePositions");
    if (!firePositions) return std::nullopt;
    offsets.inferno.fire_positions = firePositions.value();

    // SpottedState offsets
    auto spotted = getOffset("EntitySpottedState_t", "m_bSpotted");
    if (!spotted) return std::nullopt;
    offsets.spotted_state.spotted = spotted.value();

    auto spottedMask = getOffset("EntitySpottedState_t", "m_bSpottedByMask");
    if (!spottedMask) return std::nullopt;
    offsets.spotted_state.mask = spottedMask.value();

    // CameraServices offsets
    auto fov = getOffset("CCSPlayerBase_CameraServices", "m_iFOV");
    if (!fov) return std::nullopt;
    offsets.camera_services.fov = fov.value();

    // ItemServices offsets
    auto hasDefuser = getOffset("CCSPlayer_ItemServices", "m_bHasDefuser");
    if (!hasDefuser) return std::nullopt;
    offsets.item_services.has_defuser = hasDefuser.value();

    auto hasHelmet = getOffset("CCSPlayer_ItemServices", "m_bHasHelmet");
    if (!hasHelmet) return std::nullopt;
    offsets.item_services.has_helmet = hasHelmet.value();

    // WeaponServices offsets
    auto weapons = getOffset("CPlayer_WeaponServices", "m_hMyWeapons");
    if (!weapons) return std::nullopt;
    offsets.weapon_services.weapons = weapons.value();

    // ObserverServices offsets
    auto target = getOffset("CPlayer_ObserverServices", "m_hObserverTarget");
    if (!target) return std::nullopt;
    offsets.observer_services.target = target.value();

    // Weapon offsets
    auto attrManager = getOffset("C_EconEntity", "m_AttributeManager");
    if (!attrManager) return std::nullopt;
    offsets.weapon.attribute_manager = attrManager.value();

    auto item = getOffset("C_AttributeContainer", "m_Item");
    if (!item) return std::nullopt;
    offsets.weapon.item = item.value();

    auto itemDefIndex = getOffset("C_EconItemView", "m_iItemDefinitionIndex");
    if (!itemDefIndex) return std::nullopt;
    offsets.weapon.item_definition_index = itemDefIndex.value();

    // PlantedC4 offsets
    auto isTicking = getOffset("C_PlantedC4", "m_bBombTicking");
    if (!isTicking) return std::nullopt;
    offsets.planted_c4.is_ticking = isTicking.value();

    auto blowTime = getOffset("C_PlantedC4", "m_flC4Blow");
    if (!blowTime) return std::nullopt;
    offsets.planted_c4.blow_time = blowTime.value();

    auto beingDefused = getOffset("C_PlantedC4", "m_bBeingDefused");
    if (!beingDefused) return std::nullopt;
    offsets.planted_c4.being_defused = beingDefused.value();

    auto isDefused = getOffset("C_PlantedC4", "m_bBombDefused");
    if (!isDefused) return std::nullopt;
    offsets.planted_c4.is_defused = isDefused.value();

    auto hasExploded = getOffset("C_PlantedC4", "m_bHasExploded");
    if (!hasExploded) return std::nullopt;
    offsets.planted_c4.has_exploded = hasExploded.value();

    auto defuseTimeLeft = getOffset("C_PlantedC4", "m_flDefuseCountDown");
    if (!defuseTimeLeft) return std::nullopt;
    offsets.planted_c4.defuse_time_left = defuseTimeLeft.value();

    auto bombSite = getOffset("C_PlantedC4", "m_nBombSite");
    if (!bombSite) return std::nullopt;
    offsets.planted_c4.bomb_site = bombSite.value();

    // EntityIdentity size
    auto entityIdentityClass = client->getClass("CEntityIdentity");
    if (!entityIdentityClass) return std::nullopt;
    offsets.entity_identity.size = entityIdentityClass->size();

    return offsets;
}

} // namespace memory
