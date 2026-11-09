#pragma once
#include <Includes/Includes.hpp>
#include <Core/Core.hpp>
#include <vector>
#include <string>

// ── Offsets internos do CWeapon (dentro do CWeaponManager, slot ativo) ──────
// CWeaponManager + 0x20  → ptr para CWeapon (slot equipado)
// CWeapon + 0x08         → weapon hash (uint32_t)
// CWeapon + 0x198        → tint index (int32_t)
// FILL: valide 0x198 no Cheat Engine se o tint não mudar visualmente
constexpr uintptr_t CWEAPON_HASH_OFF = 0x08;
constexpr uintptr_t CWEAPON_TINT_OFF = 0x198;

namespace Core {
namespace Features {

struct WeaponSkinEntry {
    const char* displayName;
    uint32_t    weaponHash;
    int32_t     tintIndex;
};

inline const std::vector<WeaponSkinEntry> g_SkinList = {
    { "ASSAULTRIFLE - Default",        0xBFEFFF6D, 0 },
    { "ASSAULTRIFLE - Black",          0xBFEFFF6D, 1 },
    { "ASSAULTRIFLE - Army",           0xBFEFFF6D, 2 },
    { "ASSAULTRIFLE - LSPD",           0xBFEFFF6D, 3 },
    { "ASSAULTRIFLE - Angel Love",     0xBFEFFF6D, 6 },
    { "ASSAULTRIFLE - Zebra",          0xBFEFFF6D, 7 },
    { "CARBINERIFLE - Default",        0x83BF0278, 0 },
    { "CARBINERIFLE - Black",          0x83BF0278, 1 },
    { "CARBINERIFLE - Army",           0x83BF0278, 2 },
    { "PISTOL - Default",              0x1B06D571, 0 },
    { "PISTOL - Black",                0x1B06D571, 1 },
    { "PISTOL - Gold",                 0x1B06D571, 3 },
    { "PISTOL50 - Default",            0x99AEEB3B, 0 },
    { "PISTOL50 - Black",              0x99AEEB3B, 1 },
    { "SMG - Default",                 0x2BE6766B, 0 },
    { "SMG - Black",                   0x2BE6766B, 1 },
    { "MICROSMG - Default",            0x13532244, 0 },
    { "MICROSMG - Black",              0x13532244, 1 },
    { "SNIPERRIFLE - Default",         0x05FC3C11, 0 },
    { "SNIPERRIFLE - Black",           0x05FC3C11, 1 },
    { "SHOTGUN - Default",             0x1D073A89, 0 },
    { "HEAVYSNIPER - Default",         0x0EF0E9E5, 0 },
    { "RPG - Default",                 0xB1CA74B2, 0 },
};

class CSkinChanger {
public:
    int  m_selectedSkin = 0;

    uint32_t GetCurrentWeaponHash() {
        auto* wm = Core::SDK::Pointers::pLocalPlayer->GetWeaponManager();
        if (!wm) return 0;
        uintptr_t pCWeapon = Mem.Read<uintptr_t>(
            reinterpret_cast<uintptr_t>(wm) + 0x20
        );
        if (!pCWeapon) return 0;
        return Mem.Read<uint32_t>(pCWeapon + CWEAPON_HASH_OFF);
    }

    int32_t GetCurrentTint() {
        auto* wm = Core::SDK::Pointers::pLocalPlayer->GetWeaponManager();
        if (!wm) return -1;
        uintptr_t pCWeapon = Mem.Read<uintptr_t>(
            reinterpret_cast<uintptr_t>(wm) + 0x20
        );
        if (!pCWeapon) return -1;
        return Mem.Read<int32_t>(pCWeapon + CWEAPON_TINT_OFF);
    }

    bool ApplyTint(int32_t tintIndex) {
        auto* wm = Core::SDK::Pointers::pLocalPlayer->GetWeaponManager();
        if (!wm) return false;
        uintptr_t pCWeapon = Mem.Read<uintptr_t>(
            reinterpret_cast<uintptr_t>(wm) + 0x20
        );
        if (!pCWeapon) return false;
        Mem.Write<int32_t>(pCWeapon + CWEAPON_TINT_OFF, tintIndex);
        return true;
    }

    std::vector<int> GetSkinsForCurrentWeapon() {
        uint32_t curHash = GetCurrentWeaponHash();
        std::vector<int> indices;
        for (int i = 0; i < (int)g_SkinList.size(); i++) {
            if (g_SkinList[i].weaponHash == curHash)
                indices.push_back(i);
        }
        return indices;
    }
};

inline CSkinChanger g_SkinChanger;

} // Features
} // Core
