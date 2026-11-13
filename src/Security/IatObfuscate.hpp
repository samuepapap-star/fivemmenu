#pragma once
#include <windows.h>
#include <winternl.h>

namespace IAT {

    constexpr DWORD fnv1a(const char* s, DWORD h = 2166136261u) {
        return *s ? fnv1a(s + 1, (h ^ (BYTE)*s) * 16777619u) : h;
    }

    static FARPROC GetExportByHash(HMODULE hMod, DWORD targetHash) {
        auto* base = reinterpret_cast<BYTE*>(hMod);
        auto* dos  = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* nt   = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);

        const auto& expDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!expDir.VirtualAddress) return nullptr;

        auto* exp   = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + expDir.VirtualAddress);
        auto* names = reinterpret_cast<DWORD*>(base + exp->AddressOfNames);
        auto* funcs = reinterpret_cast<DWORD*>(base + exp->AddressOfFunctions);
        auto* ords  = reinterpret_cast<WORD*> (base + exp->AddressOfNameOrdinals);

        for (DWORD i = 0; i < exp->NumberOfNames; ++i) {
            const char* name = reinterpret_cast<char*>(base + names[i]);
            if (fnv1a(name) == targetHash)
                return reinterpret_cast<FARPROC>(base + funcs[ords[i]]);
        }
        return nullptr;
    }

    static HMODULE GetModuleByHash(DWORD targetHash) {
        auto* peb  = reinterpret_cast<PPEB>(__readgsqword(0x60));
        auto* head = &peb->Ldr->InMemoryOrderModuleList;
        auto* cur  = head->Flink;

        while (cur != head) {
            auto* e = CONTAINING_RECORD(cur, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
            if (e->FullDllName.Buffer) {
                char narrow[64] {};
                WideCharToMultiByte(CP_ACP, 0, e->FullDllName.Buffer, -1,
                    narrow, sizeof narrow, nullptr, nullptr);
                for (auto& c : narrow) if (c >= 'A' && c <= 'Z') c += 32;
                if (fnv1a(narrow) == targetHash)
                    return reinterpret_cast<HMODULE>(e->DllBase);
            }
            cur = cur->Flink;
        }
        return nullptr;
    }

} // namespace IAT

#define DLL_HASH(s)  IAT::fnv1a(s)
#define PROC_HASH(s) IAT::fnv1a(s)

// Uso: IAT_CALL("ntdll.dll", NtQuerySystemInformation)(args...)
#define IAT_CALL(dll, fn) \
    reinterpret_cast<decltype(&fn)>( \
        IAT::GetExportByHash( \
            IAT::GetModuleByHash(DLL_HASH(dll)), \
            PROC_HASH(#fn)))
