#pragma once
#include <Includes/Includes.hpp>
#include <Security/xorstr.hpp>
#include <winternl.h>
#include <tlhelp32.h>
#include <psapi.h>

typedef NTSTATUS ( NTAPI * _NtSetInformationProcess )(
    HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation, ULONG ProcessInformationLength );

typedef NTSTATUS ( NTAPI * _NtQuerySystemInformation )(
    ULONG SystemInformationClass, PVOID SystemInformation,
    ULONG SystemInformationLength, PULONG ReturnLength );

typedef NTSTATUS ( NTAPI * _NtQueryInformationProcess )(
    HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation, ULONG ProcessInformationLength,
    PULONG ReturnLength );

#define ProcessBreakOnTermination_Class static_cast<PROCESSINFOCLASS>( 0x1D )
#define ProcessParentPid_Class          static_cast<PROCESSINFOCLASS>( 0x26 )

namespace AcBypass {

    static _NtQuerySystemInformation g_OrigNtQSI = nullptr;
    static DWORD                     g_OurPid    = 0;

    // ── Hook: filtra handles apontando pro nosso processo na lista global ───
    NTSTATUS NTAPI HookedNtQuerySystemInformation(
        ULONG  SystemInformationClass,
        PVOID  SystemInformation,
        ULONG  SystemInformationLength,
        PULONG ReturnLength )
    {
        NTSTATUS status = g_OrigNtQSI(
            SystemInformationClass, SystemInformation,
            SystemInformationLength, ReturnLength );

        // SystemHandleInformation == 0x10
        if ( SystemInformationClass != 0x10 || !NT_SUCCESS( status ) )
            return status;

        struct SysHandleInfo {
            ULONG HandleCount;
            struct Entry {
                USHORT      ProcessId;
                USHORT      CreatorBackTraceIndex;
                UCHAR       ObjectTypeIndex;
                UCHAR       HandleAttributes;
                USHORT      Handle;
                PVOID       Object;
                ACCESS_MASK GrantedAccess;
            } Handles[ 1 ];
        };

        auto * info  = reinterpret_cast<SysHandleInfo *>( SystemInformation );
        ULONG  write = 0;

        for ( ULONG i = 0; i < info->HandleCount; i++ ) {
            auto & e = info->Handles[ i ];

            // ObjectTypeIndex 7 == Process object
            bool suspect = ( e.ObjectTypeIndex == 7 )
                        && ( e.ProcessId != g_OurPid )
                        && ( e.ProcessId != 4 );

            if ( suspect ) {
                HANDLE hSrc = OpenProcess( PROCESS_DUP_HANDLE, FALSE, e.ProcessId );
                if ( hSrc ) {
                    HANDLE hDup = nullptr;
                    if ( DuplicateHandle( hSrc, reinterpret_cast<HANDLE>( e.Handle ),
                            GetCurrentProcess( ), &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
                        bool points_to_us = ( GetProcessId( hDup ) == g_OurPid );
                        CloseHandle( hDup );
                        if ( points_to_us ) {
                            CloseHandle( hSrc );
                            continue; // oculta esta entrada
                        }
                    }
                    CloseHandle( hSrc );
                }
            }

            info->Handles[ write++ ] = e;
        }

        info->HandleCount = write;
        return status;
    }

    // ── Instala hook inline 14-byte (jmp abs 64-bit) ────────────────────────
    bool InstallQSIHook( ) {
        HMODULE ntdll = GetModuleHandleA( xorstr( "ntdll.dll" ) );
        if ( !ntdll ) return false;

        auto target = reinterpret_cast<BYTE *>(
            GetProcAddress( ntdll, xorstr( "NtQuerySystemInformation" ) ) );
        if ( !target ) return false;

        // trampolim: 5 bytes originais + jmp abs de volta ao byte 5
        static BYTE trampoline[ 14 ] = {};
        memcpy( trampoline, target, 5 );
        trampoline[ 5 ]  = 0xFF;
        trampoline[ 6 ]  = 0x25;
        *reinterpret_cast<DWORD *>( trampoline + 7 ) = 0;
        *reinterpret_cast<uintptr_t *>( trampoline + 11 ) =
            reinterpret_cast<uintptr_t>( target ) + 5;

        DWORD old;
        VirtualProtect( trampoline, sizeof trampoline, PAGE_EXECUTE_READWRITE, &old );
        g_OrigNtQSI = reinterpret_cast<_NtQuerySystemInformation>( trampoline );

        // patch: FF 25 00000000 <addr64>
        BYTE patch[ 14 ] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
        *reinterpret_cast<uintptr_t *>( patch + 6 ) =
            reinterpret_cast<uintptr_t>( &HookedNtQuerySystemInformation );

        DWORD oldProt;
        VirtualProtect( target, sizeof patch, PAGE_EXECUTE_READWRITE, &oldProt );
        memcpy( target, patch, sizeof patch );
        VirtualProtect( target, sizeof patch, oldProt, &oldProt );

        return true;
    }

    // ── Marca processo como crítico — AC evita abrir handles em proc crítico ─
    bool SetCriticalProcess( bool enable ) {
        const auto NtSIP = reinterpret_cast<_NtSetInformationProcess>(
            GetProcAddress( GetModuleHandleA( xorstr( "ntdll.dll" ) ),
                xorstr( "NtSetInformationProcess" ) ) );
        if ( !NtSIP ) return false;

        ULONG val = enable ? 1 : 0;
        return NT_SUCCESS( NtSIP( GetCurrentProcess( ),
            ProcessBreakOnTermination_Class, &val, sizeof val ) );
    }

    // ── Spoofa parent PID pra explorer.exe ──────────────────────────────────
    bool SpoofParentPid( ) {
        const auto NtSIP = reinterpret_cast<_NtSetInformationProcess>(
            GetProcAddress( GetModuleHandleA( xorstr( "ntdll.dll" ) ),
                xorstr( "NtSetInformationProcess" ) ) );
        if ( !NtSIP ) return false;

        DWORD explorerPid = 0;
        PROCESSENTRY32 pe = { sizeof( PROCESSENTRY32 ) };
        HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
        if ( snap != INVALID_HANDLE_VALUE ) {
            if ( Process32First( snap, &pe ) ) {
                do {
                    if ( _stricmp( pe.szExeFile, xorstr( "explorer.exe" ) ) == 0 ) {
                        explorerPid = pe.th32ProcessID;
                        break;
                    }
                } while ( Process32Next( snap, &pe ) );
            }
            CloseHandle( snap );
        }
        if ( !explorerPid ) return false;

        HANDLE hExplorer = OpenProcess( PROCESS_ALL_ACCESS, FALSE, explorerPid );
        if ( !hExplorer ) return false;

        NTSTATUS s = NtSIP( GetCurrentProcess( ),
            ProcessParentPid_Class, &hExplorer, sizeof hExplorer );

        CloseHandle( hExplorer );
        return NT_SUCCESS( s );
    }

    // ── Remove módulo da PEB module list ────────────────────────────────────
    void HideModuleFromPeb( HMODULE hMod ) {
        PPEB peb = reinterpret_cast<PPEB>( __readgsqword( 0x60 ) );
        auto head = &peb->Ldr->InMemoryOrderModuleList;
        auto cur  = head->Flink;

        while ( cur != head ) {
            auto entry = CONTAINING_RECORD( cur, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );
            if ( entry->DllBase == hMod ) {
                cur->Blink->Flink = cur->Flink;
                cur->Flink->Blink = cur->Blink;
                entry->InLoadOrderLinks.Blink->Flink = entry->InLoadOrderLinks.Flink;
                entry->InLoadOrderLinks.Flink->Blink = entry->InLoadOrderLinks.Blink;
                entry->InInitializationOrderLinks.Blink->Flink = entry->InInitializationOrderLinks.Flink;
                entry->InInitializationOrderLinks.Flink->Blink = entry->InInitializationOrderLinks.Blink;
                break;
            }
            cur = cur->Flink;
        }
    }

    // ── Mantém hook ativo — re-instala se restaurado ─────────────────────── 
    void MaintainThread( ) {
        HMODULE ntdll  = GetModuleHandleA( xorstr( "ntdll.dll" ) );
        auto    target = reinterpret_cast<BYTE *>(
            GetProcAddress( ntdll, xorstr( "NtQuerySystemInformation" ) ) );

        while ( true ) {
            if ( target[ 0 ] != 0xFF )
                InstallQSIHook( );
            Sleep( 800 );
        }
    }

    // ── Entry point ──────────────────────────────────────────────────────────
    void Init( HMODULE hSelf ) {
        g_OurPid = GetCurrentProcessId( );

        SpoofParentPid( );
        SetCriticalProcess( true );
        HideModuleFromPeb( hSelf );
        InstallQSIHook( );

        std::thread( MaintainThread ).detach( );
    }

} // namespace AcBypass
