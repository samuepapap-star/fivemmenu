#pragma once
#include <windows.h>
#include <Security/xorstr.hpp>

namespace KernelAC {

    static const char* const k_KnownAcDrivers[] = {
        xorstr("\\\\.\\EasyAntiCheat"),
        xorstr("\\\\.\\BEDaisy"),
        xorstr("\\\\.\\NTRootKit"),
        xorstr("\\\\.\\PROCEXP152"),
        xorstr("\\\\.\\Nal"),
        xorstr("\\\\.\\AsUpIO64"),
    };

    static bool IsDriverActive(const char* devicePath) {
        HANDLE h = CreateFileA(devicePath,
            0, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        CloseHandle(h);
        return true;
    }

    static DWORD DetectKernelAC() {
        DWORD mask = 0;
        for (DWORD i = 0; i < _countof(k_KnownAcDrivers); ++i) {
            if (IsDriverActive(k_KnownAcDrivers[i]))
                mask |= (1u << i);
        }
        return mask;
    }

    // Retorna false se EAC ou BattlEye kernel estiver ativo
    // FiveM vanilla passa sempre — AC kernel é raro em servidores Cfx
    inline bool IsSafeToInject() {
        return (DetectKernelAC() & 0x3) == 0;
    }

} // namespace KernelAC
