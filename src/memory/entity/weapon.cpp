#include "weapon.h"
#include "../process.h"
#include "../offsets.h"

namespace memory {

Weapon weaponFromHandle(uint64_t handle, const Process& process, const Offsets& offsets) {
    if (handle > UINT64_MAX - 50000) {
        return Weapon::Unknown;
    }

    uint16_t weaponIndex = process.read<uint16_t>(
        handle +
        offsets.weapon.attribute_manager +
        offsets.weapon.item +
        offsets.weapon.item_definition_index
    );

    return weaponFromIndex(weaponIndex);
}

Weapon weaponFromIndex(uint16_t index) {
    switch (index) {
        case 1: return Weapon::Deagle;
        case 2: return Weapon::DualBerettas;
        case 3: return Weapon::FiveSeven;
        case 4: return Weapon::Glock;
        case 7: return Weapon::Ak47;
        case 8: return Weapon::Aug;
        case 9: return Weapon::Awp;
        case 10: return Weapon::Famas;
        case 11: return Weapon::G3SG1;
        case 13: return Weapon::Galilar;
        case 14: return Weapon::M249;
        case 16: return Weapon::M4A4;
        case 17: return Weapon::Mac10;
        case 19: return Weapon::P90;
        case 23: return Weapon::Mp5Sd;
        case 24: return Weapon::Ump45;
        case 25: return Weapon::Xm1014;
        case 26: return Weapon::Bizon;
        case 27: return Weapon::Mag7;
        case 28: return Weapon::Negev;
        case 29: return Weapon::Sawedoff;
        case 30: return Weapon::Tec9;
        case 31: return Weapon::Taser;
        case 32: return Weapon::P2000;
        case 33: return Weapon::Mp7;
        case 34: return Weapon::Mp9;
        case 35: return Weapon::Nova;
        case 36: return Weapon::P250;
        case 38: return Weapon::Scar20;
        case 39: return Weapon::Sg556;
        case 40: return Weapon::Ssg08;
        case 41:
        case 42:
        case 59:
        case 60: // M4A1
        case 80:
        case 500:
        case 505:
        case 506:
        case 507:
        case 508:
        case 509:
        case 512:
        case 514:
        case 515:
        case 516:
        case 519:
        case 520:
        case 522:
        case 523:
            return Weapon::Knife;
        case 43: return Weapon::Flashbang;
        case 44: return Weapon::HeGrenade;
        case 45: return Weapon::Smoke;
        case 46: return Weapon::Molotov;
        case 47: return Weapon::Decoy;
        case 48: return Weapon::Incendiary;
        case 49: return Weapon::C4;
        case 61: return Weapon::Usp;
        case 63: return Weapon::Cz75A;
        case 64: return Weapon::Revolver;
        default: return Weapon::Unknown;
    }
}

const char* weaponToString(Weapon weapon) {
    switch (weapon) {
        case Weapon::Unknown: return "Unknown";
        case Weapon::Knife: return "Knife";
        case Weapon::Cz75A: return "CZ75-Auto";
        case Weapon::Deagle: return "Desert Eagle";
        case Weapon::DualBerettas: return "Dual Berettas";
        case Weapon::FiveSeven: return "Five-SeveN";
        case Weapon::Glock: return "Glock-18";
        case Weapon::P2000: return "P2000";
        case Weapon::P250: return "P250";
        case Weapon::Revolver: return "R8 Revolver";
        case Weapon::Tec9: return "Tec-9";
        case Weapon::Usp: return "USP-S";
        case Weapon::Bizon: return "PP-Bizon";
        case Weapon::Mac10: return "MAC-10";
        case Weapon::Mp5Sd: return "MP5-SD";
        case Weapon::Mp7: return "MP7";
        case Weapon::Mp9: return "MP9";
        case Weapon::P90: return "P90";
        case Weapon::Ump45: return "UMP-45";
        case Weapon::M249: return "M249";
        case Weapon::Negev: return "Negev";
        case Weapon::Mag7: return "MAG-7";
        case Weapon::Nova: return "Nova";
        case Weapon::Sawedoff: return "Sawed-Off";
        case Weapon::Xm1014: return "XM1014";
        case Weapon::Ak47: return "AK-47";
        case Weapon::Aug: return "AUG";
        case Weapon::Famas: return "FAMAS";
        case Weapon::Galilar: return "Galil AR";
        case Weapon::M4A4: return "M4A4";
        case Weapon::M4A1: return "M4A1-S";
        case Weapon::Sg556: return "SG 553";
        case Weapon::Awp: return "AWP";
        case Weapon::G3SG1: return "G3SG1";
        case Weapon::Scar20: return "SCAR-20";
        case Weapon::Ssg08: return "SSG 08";
        case Weapon::Taser: return "Zeus x27";
        case Weapon::Flashbang: return "Flashbang";
        case Weapon::HeGrenade: return "HE Grenade";
        case Weapon::Smoke: return "Smoke Grenade";
        case Weapon::Molotov: return "Molotov Cocktail";
        case Weapon::Decoy: return "Decoy Grenade";
        case Weapon::Incendiary: return "Incendiary Grenade";
        case Weapon::C4: return "C4 Explosive";
        default: return "Unknown";
    }
}

const char* weaponToIcon(Weapon weapon) {
    switch (weapon) {
        case Weapon::Unknown: return "";
        case Weapon::Knife: return "\xEE\x81\x92";  // U+E052
        case Weapon::Cz75A: return "\xEE\x80\xAB";  // U+E02B
        case Weapon::Deagle: return "\xEE\x81\x8A";  // U+E04A
        case Weapon::DualBerettas: return "\xEE\x81\x96";  // U+E056
        case Weapon::FiveSeven: return "\xEE\x80\xBA";  // U+E03A
        case Weapon::Glock: return "\xEE\x80\xA3";  // U+E023
        case Weapon::P2000: return "\xEE\x81\x9A";  // U+E05A
        case Weapon::P250: return "\xEE\x81\x8F";  // U+E04F
        case Weapon::Revolver: return "\xEE\x80\x83";  // U+E003
        case Weapon::Tec9: return "\xEE\x80\x8E";  // U+E00E
        case Weapon::Usp: return "\xEE\x80\xB7";  // U+E037
        case Weapon::Bizon: return "\xEE\x80\x8C";  // U+E00C
        case Weapon::Mac10: return "\xEE\x81\x89";  // U+E049
        case Weapon::Mp5Sd: return "\xEE\x80\xB9";  // U+E039
        case Weapon::Mp7: return "\xEE\x81\x87";  // U+E047
        case Weapon::Mp9: return "\xEE\x81\x80";  // U+E040
        case Weapon::P90: return "\xEE\x80\xB5";  // U+E035
        case Weapon::Ump45: return "\xEE\x80\x9F";  // U+E01F
        case Weapon::M249: return "\xEE\x81\x95";  // U+E055
        case Weapon::Negev: return "\xEE\x80\x99";  // U+E019
        case Weapon::Mag7: return "\xEE\x80\xB2";  // U+E032
        case Weapon::Nova: return "\xEE\x80\xAD";  // U+E02D
        case Weapon::Sawedoff: return "\xEE\x80\xB1";  // U+E031
        case Weapon::Xm1014: return "\xEE\x80\x9D";  // U+E01D
        case Weapon::Ak47: return "\xEE\x80\x88";  // U+E008
        case Weapon::Aug: return "\xEE\x81\xA0";  // U+E060
        case Weapon::Famas: return "\xEE\x80\xA5";  // U+E025
        case Weapon::Galilar: return "\xEE\x80\x80";  // U+E000
        case Weapon::M4A4: return "\xEE\x81\x82";  // U+E042
        case Weapon::M4A1: return "\xEE\x80\x95";  // U+E015
        case Weapon::Sg556: return "\xEE\x80\x8D";  // U+E00D
        case Weapon::Awp: return "\xEE\x80\x87";  // U+E007
        case Weapon::G3SG1: return "\xEE\x81\x8B";  // U+E04B
        case Weapon::Scar20: return "\xEE\x80\x89";  // U+E009
        case Weapon::Ssg08: return "\xEE\x80\xBE";  // U+E03E
        case Weapon::Taser: return "\xEE\x80\xA1";  // U+E021
        case Weapon::Flashbang: return "\xEE\x81\x9E";  // U+E05E
        case Weapon::HeGrenade: return "\xEE\x80\x8B";  // U+E00B
        case Weapon::Smoke: return "\xEE\x80\xAE";  // U+E02E
        case Weapon::Molotov: return "\xEE\x80\xA4";  // U+E024
        case Weapon::Decoy: return "\xEE\x80\xA8";  // U+E028
        case Weapon::Incendiary: return "\xEE\x80\x9A";  // U+E01A
        case Weapon::C4: return "\xEE\x80\x9E";  // U+E01E
        default: return "";
    }
}

} // namespace memory
