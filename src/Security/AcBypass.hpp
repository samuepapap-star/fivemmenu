#pragma once
#include <Includes/Includes.hpp>
#include <Security/xorstr.hpp>
#include <Security/IatObfuscate.hpp>
#include <Security/StackSpoof.hpp>
#include <Security/KernelAcDetect.hpp>
#include <winternl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <atomic>

// winternl.h expõe LDR_DATA_TABLE_ENTRY incompleto — redefinição completa
struct LDR_ENTRY_FULL {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID      DllBase;
    PVOID      EntryPoint;
    ULONG      SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG      Flags;
    USHORT     LoadCount;
    USHORT     TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG      TimeDateStamp;
};

typedef NTSTATUS (NTAPI* _NtSetInfoProc)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
typedef NTSTATUS (NTAPI* _NtQuerySysInfo)(ULONG, PVOID, ULONG, PULONG);

#define PROC_PARENT_PID static_cast<PROCESSINFOCLASS>(0x26)

// ── Bloco de offsets — edite aqui se o FiveM atualizar ────────────────────
namespace off {
    constexpr uintptr_t kReplayInterface = 0x1FBD4F0;
    constexpr uintptr_t kBlipList        = 0x2023400;
    constexpr uintptr_t kCamera          = 0x201ED50;
    constexpr uintptr_t kBullet          = 0x101A660;

    constexpr ptrdiff_t kPedPos          = 0x28;
    constexpr ptrdiff_t kPedHealth       = 0x2A4;
    constexpr ptrdiff_t kPedMaxHealth    = 0x2A8;
    constexpr ptrdiff_t kPedArmor        = 0xDA0;
    constexpr ptrdiff_t kPedPlayerInfo   = 0x28C;
    constexpr ptrdiff_t kPedVehicle      = 0xD04;

    constexpr ptrdiff_t kVehPos          = 0x30;
    constexpr ptrdiff_t kVehEngineHealth = 0x28C;
    constexpr ptrdiff_t kVehBodyHealth   = 0x290;
    constexpr ptrdiff_t kVehFuelHealth   = 0x2AC;
    constexpr ptrdiff_t kVehHandling     = 0x7DC;
    constexpr ptrdiff_t kVehVelocity     = 0x7E4;

    constexpr ptrdiff_t kWeapClipAmmo    = 0x5C;
    constexpr ptrdiff_t kWeapAmmoCount   = 0x60;
    constexpr ptrdiff_t kWeapInfoDmg     = 0x18;
    constexpr ptrdiff_t kWeapInfoSpread  = 0x78;
    constexpr ptrdiff_t kWeapInfoRecoil  = 0x7C;

    constexpr ptrdiff_t kViewMatrix      = 0x24C0;

    constexpr ptrdiff_t kPedPool         = 0x18;
    constexpr ptrdiff_t kVehiclePool     = 0x58;
    constexpr ptrdiff_t kObjectPool      = 0x70;
    constexpr ptrdiff_t kPickupPool      = 0xA0;
}

namespace AcBypass {

    static _NtQuerySysInfo       g_OrigNtQSI   = nullptr;
    static std::atomic<DWORD>    g_OurPid      { 0 };
    static std::atomic<bool>     g_Running     { true };

    // ── Trampolim alocado com VirtualAlloc ────────────────────────────────
    struct Trampoline {
        BYTE  code[14];
        bool  installed = false;
    };

    static Trampoline* g_Tramp = nullptr;

    static bool BuildTrampoline(BYTE* target) {
        if (!g_Tramp) {
            g_Tramp = static_cast<Trampoline*>(
                VirtualAlloc(nullptr, sizeof(Trampoline),
                    MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
            if (!g_Tramp) return false;
        }

        memcpy(g_Tramp->code, target, 5);

        // JMP abs 64-bit de volta ao byte 5 do original
        g_Tramp->code[5]  = 0xFF;
        g_Tramp->code[6]  = 0x25;
        *reinterpret_cast<DWORD*>(g_Tramp->code + 7)      = 0;
        *reinterpret_cast<uintptr_t*>(g_Tramp->code + 11) =
            reinterpret_cast<uintptr_t>(target) + 5;

        g_OrigNtQSI = reinterpret_cast<_NtQuerySysInfo>(
            reinterpret_cast<void*>(g_Tramp->code));

        g_Tramp->installed = true;
        return true;
    }

    // ── Hook NtQuerySystemInformation ─────────────────────────────────────
    NTSTATUS NTAPI HookedNtQSI(
        ULONG cls, PVOID buf, ULONG len, PULONG ret)
    {
        NTSTATUS s = g_OrigNtQSI(cls, buf, len, ret);

        if (cls != 0x10 || !NT_SUCCESS(s)) return s;

        struct SHI {
            ULONG HandleCount;
            struct Entry {
                USHORT      ProcessId;
                USHORT      CreatorBackTrace;
                UCHAR       ObjectType;
                UCHAR       Attributes;
                USHORT      Handle;
                PVOID       Object;
                ACCESS_MASK Access;
            } Handles[1];
        };

        const DWORD ourPid = g_OurPid.load(std::memory_order_relaxed);
        auto* info  = static_cast<SHI*>(buf);
        ULONG write = 0;

        for (ULONG i = 0; i < info->HandleCount; ++i) {
            auto& e = info->Handles[i];

            if (e.ObjectType == 7
                && e.ProcessId != ourPid
                && e.ProcessId != 4)
            {
                HANDLE hSrc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, e.ProcessId);
                if (hSrc) {
                    HANDLE hDup = nullptr;
                    if (DuplicateHandle(hSrc,
                            reinterpret_cast<HANDLE>(static_cast<uintptr_t>(e.Handle)),
                            GetCurrentProcess(), &hDup,
                            0, FALSE, DUPLICATE_SAME_ACCESS)) {
                        const bool us = (GetProcessId(hDup) == ourPid);
                        CloseHandle(hDup);
                        CloseHandle(hSrc);
                        if (us) continue;
                    } else {
                        CloseHandle(hSrc);
                    }
                }
            }
            info->Handles[write++] = e;
        }
        info->HandleCount = write;
        return s;
    }

    // ── Instala patch FF25 de 14 bytes ────────────────────────────────────
    bool InstallQSIHook() {
        HMODULE ntdll = GetModuleHandleA(xorstr("ntdll.dll"));
        if (!ntdll) return false;

        auto* tgt = reinterpret_cast<BYTE*>(
            GetProcAddress(ntdll, xorstr("NtQuerySystemInformation")));
        if (!tgt) return false;

        if (!g_Tramp || !g_Tramp->installed)
            if (!BuildTrampoline(tgt)) return false;

        BYTE patch[14] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
        *reinterpret_cast<uintptr_t*>(patch + 6) =
            reinterpret_cast<uintptr_t>(&HookedNtQSI);

        DWORD old;
        VirtualProtect(tgt, sizeof patch, PAGE_EXECUTE_READWRITE, &old);
        memcpy(tgt, patch, sizeof patch);
        VirtualProtect(tgt, sizeof patch, old, &old);
        return true;
    }

    // ── Spoofa parent PID pra explorer.exe ───────────────────────────────
    void SpoofParentPid() {
        const auto fn = reinterpret_cast<_NtSetInfoProc>(
            GetProcAddress(GetModuleHandleA(xorstr("ntdll.dll")),
                xorstr("NtSetInformationProcess")));
        if (!fn) return;

        DWORD explorerPid = 0;
        PROCESSENTRY32W pe { sizeof pe };
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, xorstr(L"explorer.exe")) == 0) {
                        explorerPid = pe.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
        }
        if (!explorerPid) return;

        HANDLE hExp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, explorerPid);
        if (!hExp) return;
        fn(GetCurrentProcess(), PROC_PARENT_PID, &hExp, sizeof hExp);
        CloseHandle(hExp);
    }

    // ── Esconde DLL da PEB — todas as listas incluindo HashLinks ──────────
    void HideModuleFromPeb(HMODULE hMod) {
        auto* peb  = reinterpret_cast<PPEB>(__readgsqword(0x60));
        auto* head = &peb->Ldr->InMemoryOrderModuleList;
        auto* cur  = head->Flink;

        while (cur != head) {
            auto* e    = CONTAINING_RECORD(cur, LDR_ENTRY_FULL, InMemoryOrderLinks);
            auto* next = cur->Flink;

            if (e->DllBase == hMod) {
                e->InMemoryOrderLinks.Blink->Flink       = e->InMemoryOrderLinks.Flink;
                e->InMemoryOrderLinks.Flink->Blink       = e->InMemoryOrderLinks.Blink;
                e->InLoadOrderLinks.Blink->Flink         = e->InLoadOrderLinks.Flink;
                e->InLoadOrderLinks.Flink->Blink         = e->InLoadOrderLinks.Blink;
                e->InInitializationOrderLinks.Blink->Flink = e->InInitializationOrderLinks.Flink;
                e->InInitializationOrderLinks.Flink->Blink = e->InInitializationOrderLinks.Blink;
                e->HashLinks.Blink->Flink                = e->HashLinks.Flink;
                e->HashLinks.Flink->Blink                = e->HashLinks.Blink;
                break;
            }
            cur = next;
        }
    }

    // ── Zera nomes de seção PE em memória ─────────────────────────────────
    void ObfuscateSectionNames(HMODULE hMod) {
        auto* base = reinterpret_cast<BYTE*>(hMod);
        auto* dos  = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        auto* nt   = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        auto* sec  = IMAGE_FIRST_SECTION(nt);
        const WORD n = nt->FileHeader.NumberOfSections;

        DWORD old;
        VirtualProtect(sec, n * sizeof(IMAGE_SECTION_HEADER), PAGE_READWRITE, &old);
        for (WORD i = 0; i < n; ++i)
            memset(sec[i].Name, 0, IMAGE_SIZEOF_SHORT_NAME);
        VirtualProtect(sec, n * sizeof(IMAGE_SECTION_HEADER), old, &old);
    }

    // ── MaintainThread — checa 2 bytes do patch ───────────────────────────
    static void MaintainThread() {
        HMODULE ntdll = GetModuleHandleA(xorstr("ntdll.dll"));
        auto* tgt = reinterpret_cast<BYTE*>(
            GetProcAddress(ntdll, xorstr("NtQuerySystemInformation")));

        while (g_Running.load(std::memory_order_relaxed)) {
            if (tgt[0] != 0xFF || tgt[1] != 0x25)
                InstallQSIHook();
            Sleep(600);
        }
    }

    // ── Entry point ───────────────────────────────────────────────────────
    // BSOD FIX: SetCriticalProcess REMOVIDO
    void Init(HMODULE hSelf) {
        g_OurPid.store(GetCurrentProcessId(), std::memory_order_relaxed);

        // Detecta AC kernel — sai silencioso se EAC/BattlEye estiver ativo
        if (!KernelAC::IsSafeToInject())
            return;

        StackSpoof::Init();
        SpoofParentPid();
        HideModuleFromPeb(hSelf);
        ObfuscateSectionNames(hSelf);
        InstallQSIHook();

        // Job object — fecha limpo sem BSOD
        HANDLE hJob = CreateJobObjectA(nullptr, nullptr);
        if (hJob) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli {};
            jeli.BasicLimitInformation.LimitFlags =
                JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
            SetInformationJobObject(hJob, JobObjectExtendedLimitInformation,
                &jeli, sizeof jeli);
            AssignProcessToJobObject(hJob, GetCurrentProcess());
        }

        std::thread(MaintainThread).detach();
    }

    // ── Chama no DLL_PROCESS_DETACH ───────────────────────────────────────
    void Shutdown() {
        g_Running.store(false, std::memory_order_relaxed);
    }

} // namespace AcBypass
