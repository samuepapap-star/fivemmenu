#pragma once
#include <Core/SDK/LuaExecutor.hpp>
#include <cstdio>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  NativeBridge  —  High-level wrappers over LuaExecutor
//
//  All functions call Execute() (sync) or ExecuteAsync().
//  Sync  → safe to call from menu logic threads, blocks up to ~2s.
//  Async → safe to call from render loop (fire-and-forget detached thread).
//
//  FiveM Lua naming:
//    PlayerPedId()                     — local player ped handle
//    GetSelectedPedWeapon(ped)         — current weapon hash
//    GetAmmoInPedWeapon(ped, hash)     — current ammo count
//    SetPedAmmo(ped, hash, amount)     — set ammo for specific weapon
//    AddAmmoToPed(ped, hash, amount)   — add ammo for specific weapon
//    SetEntityHealth(ped, hp)          — set health (100 = full, 200 = max)
//    SetPedArmour(ped, amount)         — set armor (0–100)
//    SetPlayerInvincible(player, bool) — god mode (uses PlayerId())
//    SetEntityVisible(ped, bool, false)— visibility toggle
//    SetEntityCoords(ped, x, y, z, ...) — teleport
//    GiveWeaponToPed(ped, hash, ammo, equipNow, ammoInClip)
//    RemoveAllPedWeapons(ped, true)    — strip all weapons
//    SetPedMaxHealth(ped, hp)          — set max health cap
//    NetworkRequestControlOfEntity(ped)— request network control
// ─────────────────────────────────────────────────────────────────────────────

namespace Core { namespace Features {

namespace NativeBridge
{
    using Exec = Core::SDK::LuaExecutor;

    // ── Ammo ─────────────────────────────────────────────────────────────────

    // Set ammo for the currently selected weapon
    inline bool SetCurrentWeaponAmmo( int amount )
    {
        char code[256];
        snprintf( code, sizeof(code),
            "local ped = PlayerPedId() "
            "local wep = GetSelectedPedWeapon(ped) "
            "SetPedAmmo(ped, wep, %d)", amount );
        return Exec::Execute( code );
    }

    // Add ammo to the currently selected weapon
    inline bool AddCurrentWeaponAmmo( int amount )
    {
        char code[256];
        snprintf( code, sizeof(code),
            "local ped = PlayerPedId() "
            "local wep = GetSelectedPedWeapon(ped) "
            "AddAmmoToPed(ped, wep, %d)", amount );
        return Exec::Execute( code );
    }

    // Set ammo for a specific weapon hash
    inline bool SetWeaponAmmo( unsigned int weaponHash, int amount )
    {
        char code[256];
        snprintf( code, sizeof(code),
            "local ped = PlayerPedId() "
            "if HasPedGotWeapon(ped, 0x%08X, false) then "
            "  SetPedAmmo(ped, 0x%08X, %d) "
            "end",
            weaponHash, weaponHash, amount );
        return Exec::Execute( code );
    }

    // Refill ALL carried weapons to 9999 ammo (async — safe to call each tick)
    inline void RefillAllAmmoAsync()
    {
        Exec::ExecuteAsync(
            "local ped = PlayerPedId() "
            "local wep = GetSelectedPedWeapon(ped) "
            "SetPedAmmo(ped, wep, 9999)" );
    }

    // One-shot refill every weapon the ped carries
    inline bool RefillAllAmmoSync()
    {
        return Exec::Execute(
            "local ped = PlayerPedId() "
            "local wep = GetSelectedPedWeapon(ped) "
            "SetPedAmmo(ped, wep, 9999)" );
    }

    // ── Health / Armor ────────────────────────────────────────────────────────

    // Set health (200 = full max, 100 = default max without upgrades)
    inline bool SetHealth( int health )
    {
        char code[128];
        snprintf( code, sizeof(code),
            "local ped = PlayerPedId() "
            "SetEntityHealth(ped, %d)", health );
        return Exec::Execute( code );
    }

    // Set armor (0–100)
    inline bool SetArmor( int armor )
    {
        char code[128];
        snprintf( code, sizeof(code),
            "SetPedArmour(PlayerPedId(), %d)", armor );
        return Exec::Execute( code );
    }

    // Full heal — max health + max armor
    inline bool FullHeal()
    {
        return Exec::Execute(
            "local ped = PlayerPedId() "
            "SetEntityHealth(ped, 200) "
            "SetPedArmour(ped, 100)" );
    }

    // ── God Mode ─────────────────────────────────────────────────────────────

    // SetPlayerInvincible(PlayerId(), bool)
    inline bool SetGodMode( bool enabled )
    {
        char code[128];
        snprintf( code, sizeof(code),
            "SetPlayerInvincible(PlayerId(), %s)", enabled ? "true" : "false" );
        return Exec::Execute( code );
    }

    // ── Visibility ───────────────────────────────────────────────────────────

    // SetEntityVisible — false = invisible to other players & NPCs
    inline bool SetVisible( bool visible )
    {
        char code[128];
        snprintf( code, sizeof(code),
            "SetEntityVisible(PlayerPedId(), %s, false)", visible ? "true" : "false" );
        return Exec::Execute( code );
    }

    // ── Teleport ─────────────────────────────────────────────────────────────

    inline bool Teleport( float x, float y, float z )
    {
        char code[256];
        snprintf( code, sizeof(code),
            "SetEntityCoords(PlayerPedId(), %.4f, %.4f, %.4f, false, false, false, true)",
            x, y, z );
        return Exec::Execute( code );
    }

    // ── Weapons ──────────────────────────────────────────────────────────────

    // Give a single weapon with ammo
    inline bool GiveWeapon( unsigned int hash, int ammo, bool equipNow = true )
    {
        char code[256];
        snprintf( code, sizeof(code),
            "GiveWeaponToPed(PlayerPedId(), 0x%08X, %d, false, %s)",
            hash, ammo, equipNow ? "true" : "false" );
        return Exec::Execute( code );
    }

    // Give a full loadout (common weapons)
    inline bool GiveAllWeapons()
    {
        return Exec::Execute(
            "local ped = PlayerPedId() "
            // Pistols
            "GiveWeaponToPed(ped, 0x92A27487, 999, false, false) "  // WEAPON_PISTOL
            "GiveWeaponToPed(ped, 0xBFD21232, 999, false, false) "  // WEAPON_COMBATPISTOL
            // SMG
            "GiveWeaponToPed(ped, 0x2BE6766B, 999, false, false) "  // WEAPON_SMG
            "GiveWeaponToPed(ped, 0xEFE7E2DF, 999, false, false) "  // WEAPON_MICROSMG
            // Rifles
            "GiveWeaponToPed(ped, 0xBFEF0C6D, 999, false, false) "  // WEAPON_ASSAULTRIFLE
            "GiveWeaponToPed(ped, 0x394F415C, 999, false, false) "  // WEAPON_CARBINERIFLE
            // Sniper
            "GiveWeaponToPed(ped, 0x05FC3C11, 999, false, false) "  // WEAPON_SNIPERRIFLE
            // Shotgun
            "GiveWeaponToPed(ped, 0x1D073A89, 999, false, false) "  // WEAPON_PUMPSHOTGUN
            // Heavy
            "GiveWeaponToPed(ped, 0xB1CA77B1, 999, false, false) "  // WEAPON_MINIGUN
            "GiveWeaponToPed(ped, 0xFBAB5776, 999, false, false) "  // WEAPON_RPG
            // Melee
            "GiveWeaponToPed(ped, 0x99B507EA, 1,   false, false) "  // WEAPON_KNIFE
            // set current weapon ammo full
            "local wep = GetSelectedPedWeapon(ped) "
            "SetPedAmmo(ped, wep, 9999)"
        );
    }

    // Strip all weapons
    inline bool RemoveAllWeapons()
    {
        return Exec::Execute( "RemoveAllPedWeapons(PlayerPedId(), true)" );
    }

    // ── Freeze / Unfreeze ─────────────────────────────────────────────────────

    inline bool FreezePlayer( bool freeze )
    {
        char code[128];
        snprintf( code, sizeof(code),
            "FreezeEntityPosition(PlayerPedId(), %s)", freeze ? "true" : "false" );
        return Exec::Execute( code );
    }

    // ── Wanted level ─────────────────────────────────────────────────────────

    inline bool ClearWanted()
    {
        return Exec::Execute(
            "local pid = PlayerId() "
            "SetPlayerWantedLevel(pid, 0, false) "
            "SetPlayerWantedLevelNow(pid, false)" );
    }

    inline bool SetWanted( int level )
    {
        char code[128];
        snprintf( code, sizeof(code),
            "SetPlayerWantedLevel(PlayerId(), %d, false) "
            "SetPlayerWantedLevelNow(PlayerId(), false)", level );
        return Exec::Execute( code );
    }

    // ── Noclip helpers (use alongside existing NoclipVisibility logic) ────────

    // Disable entity collision (soft noclip)
    inline bool SetCollision( bool enabled )
    {
        char code[128];
        snprintf( code, sizeof(code),
            "SetEntityCollision(PlayerPedId(), %s, %s)",
            enabled ? "true" : "false",
            enabled ? "true" : "false" );
        return Exec::Execute( code );
    }

    // ── Custom Lua ────────────────────────────────────────────────────────────

    // Execute arbitrary Lua code — synchronous
    inline bool Raw( const std::string& lua, int timeoutMs = 2000 )
    {
        return Exec::Execute( lua, timeoutMs );
    }

    // Execute arbitrary Lua code — fire-and-forget
    inline void RawAsync( const std::string& lua )
    {
        Exec::ExecuteAsync( lua );
    }

} // namespace NativeBridge

} } // namespace Core::Features
