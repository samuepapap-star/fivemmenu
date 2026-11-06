#pragma once
#include <Core/SDK/SDK.hpp>

namespace Core { namespace Features { namespace NoclipVisibility {
// Independent player visibility control adapted from SHARK. Does not alter noclip.
// Keep its original values so disabling the option does not force default values.
template<class T> inline bool Read(uintptr_t address, T& value) {
    SIZE_T bytes = 0;
    return address >= 0x10000 && Mem.ProcHandle &&
        ReadProcessMemory(Mem.ProcHandle, reinterpret_cast<LPCVOID>(address),
            &value, sizeof(value), &bytes) && bytes == sizeof(value);
}
template<class T> inline bool Write(uintptr_t address, const T& value) {
    SIZE_T bytes = 0;
    return address >= 0x10000 && Mem.ProcHandle &&
        WriteProcessMemory(Mem.ProcHandle, reinterpret_cast<LPVOID>(address),
            &value, sizeof(value), &bytes) && bytes == sizeof(value);
}
struct SavedState {
    uintptr_t ped = 0, netObject = 0;
    DWORD processId = 0;
    BYTE alpha = 255, invisible = 0;
};
inline SavedState saved;

inline void Restore(uintptr_t currentPed, DWORD currentProcess) {
    // Discard stale entities instead of writing into a respawned/recycled ped.
    if (saved.ped && saved.ped == currentPed && saved.processId == currentProcess) {
        uintptr_t netObject = 0;
        BYTE current = 0;
        if (Read(saved.ped + 0xD0, netObject) && netObject == saved.netObject) {
            if (Read(saved.netObject + 0xD1, current) && current == 1)
                Write(saved.netObject + 0xD1, saved.invisible);
        }
        if (Read(saved.ped + 0xAC, current) && current == 160)
            Write(saved.ped + 0xAC, saved.alpha);
    }
    saved = SavedState{};
}

inline void Tick(bool forceRestore = false) {
    const uintptr_t ped = reinterpret_cast<uintptr_t>(Core::SDK::Pointers::pLocalPlayer);
    const DWORD processId = Mem.ProcHandle ? GetProcessId(Mem.ProcHandle) : 0;
    const bool enabled = !forceRestore && g_MenuInfo.IsLogged && processId && ped &&
        g_Config.Player->InvisibleEnabled;
    if (!enabled) { Restore(ped, processId); return; }

    uintptr_t netObject = 0;
    if (!Read(ped + 0xD0, netObject) || netObject < 0x10000) {
        Restore(ped, processId); return;
    }
    if (saved.ped && (saved.ped != ped || saved.netObject != netObject || saved.processId != processId))
        Restore(ped, processId);
    if (!saved.ped) {
        SavedState original;
        original.ped = ped; original.netObject = netObject; original.processId = processId;
        if (!Read(ped + 0xAC, original.alpha) || !Read(netObject + 0xD1, original.invisible) ||
            original.invisible > 1) return;
        saved = original;
    }
    // Same values as SHARK: network invisibility flag and local semi-transparency.
    // Byte-write success does not establish visibility to other clients.
    if (!Write(netObject + 0xD1, BYTE(1)) || !Write(ped + 0xAC, BYTE(160)))
        Restore(ped, processId);
}
}}}
