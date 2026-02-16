#pragma once

#include <string>

namespace memory {

enum class WeaponClass {
    Unknown,
    Knife,
    Pistol,
    Smg,
    Heavy,    // Negev and M249
    Shotgun,
    Rifle,    // All rifles except snipers
    Sniper,   // Require different handling in aimbot
    Grenade,
    Utility   // Taser
};

// Get weapon class from weapon name string
inline WeaponClass weaponClassFromString(const std::string& name) {
    // Knives
    if (name == "bayonet" || name == "knife" || name == "knife_bowie" ||
        name == "knife_butterfly" || name == "knife_canis" || name == "knife_cord" ||
        name == "knife_css" || name == "knife_falchion" || name == "knife_flip" ||
        name == "knife_gut" || name == "knife_gypsy_jackknife" || name == "knife_karambit" ||
        name == "knife_kukri" || name == "knife_m9_bayonet" || name == "knife_outdoor" ||
        name == "knife_push" || name == "knife_skeleton" || name == "knife_stiletto" ||
        name == "knife_survival_bowie" || name == "knife_t" || name == "knife_tactical" ||
        name == "knife_twinblade" || name == "knife_ursus" || name == "knife_widowmaker") {
        return WeaponClass::Knife;
    }

    // Pistols
    if (name == "cz75a" || name == "deagle" || name == "elite" || name == "fiveseven" ||
        name == "glock" || name == "hkp2000" || name == "p250" || name == "revolver" ||
        name == "tec9" || name == "usp_silencer" || name == "usp_silencer_off") {
        return WeaponClass::Pistol;
    }

    // SMGs
    if (name == "bizon" || name == "mac10" || name == "mp5sd" || name == "mp7" ||
        name == "mp9" || name == "p90" || name == "ump45") {
        return WeaponClass::Smg;
    }

    // LMGs/Heavy
    if (name == "m249" || name == "negev") {
        return WeaponClass::Heavy;
    }

    // Shotguns
    if (name == "mag7" || name == "nova" || name == "sawedoff" || name == "xm1014") {
        return WeaponClass::Shotgun;
    }

    // Rifles
    if (name == "ak47" || name == "aug" || name == "famas" || name == "galilar" ||
        name == "m4a1_silencer" || name == "m4a1_silencer_off" || name == "m4a1" ||
        name == "sg556") {
        return WeaponClass::Rifle;
    }

    // Snipers
    if (name == "awp" || name == "g3sg1" || name == "scar20" || name == "ssg08") {
        return WeaponClass::Sniper;
    }

    // Grenades
    if (name == "flashbang" || name == "hegrenade" || name == "smokegrenade" ||
        name == "molotov" || name == "decoy" || name == "incgrenade") {
        return WeaponClass::Grenade;
    }

    // Utility
    if (name == "taser") {
        return WeaponClass::Utility;
    }

    return WeaponClass::Unknown;
}

} // namespace memory
