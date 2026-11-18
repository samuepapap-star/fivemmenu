#pragma once
#include <Includes/Includes.hpp>
#include <Core/SDK/Memory.hpp>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  LuaExecutor  —  FiveM external Lua code execution via CreateRemoteThread
//
//  Strategy:
//    1. Locate the lua54 module in the target process (several name variants)
//    2. Read its PE export table (via RPM) to get luaL_loadbuffer + lua_pcall
//    3. Scan process heap pages for a valid lua_State (Lua 5.4 header checks)
//    4. Test each candidate: PlayerPedId() — only the FiveM citizen context has it
//    5. Execute arbitrary Lua via shellcode + CreateRemoteThread (same pattern
//       as GiveWeapon.hpp — no additional dependencies needed)
//
//  Shellcode layout in remote RWX alloc:
//    [shellcode bytes]  → padded to 16-byte boundary
//    [ExecArgs struct]  → 56 bytes, rcx = pointer to this
//    [Lua code string]  → null-terminated, referenced by ExecArgs.codePtr
//
//  ExecArgs memory map (fields match shellcode MOV offsets):
//    +0x00  uint64 fnLoadBuffer   call [rbx+0x00]
//    +0x08  uint64 fnPcall        call [rbx+0x08]
//    +0x10  uint64 luaState       mov rcx, [rbx+0x10]
//    +0x18  uint64 codePtr        mov rdx, [rbx+0x18]
//    +0x20  uint32 codeLen        mov r8d, [rbx+0x20]
//    +0x24  int32  execResult     shellcode writes pcallk result here (-1 = not run)
//    +0x28  char[16] chunkName    lea r9,  [rbx+0x28]
//    +0x38  (Lua code string follows here in the allocation)
// ─────────────────────────────────────────────────────────────────────────────

namespace Core { namespace SDK {

class LuaExecutor
{
public:
    static bool      s_Ready;
    static uintptr_t s_LuaState;
    static uintptr_t s_fnLoadBuffer;
    static uintptr_t s_fnPcall;
    static bool      s_Initializing;

    // ── Public API ──────────────────────────────────────────────────────────

    // Synchronous init — call once from a non-render thread (or on demand).
    // Scans for Lua module + state; returns true if executor is ready.
    static bool Init();

    // Background init — fires off a thread and returns immediately.
    // Check IsReady() before calling Execute().
    static void InitAsync();

    // Execute Lua code synchronously; returns true if lua_pcallk returned LUA_OK.
    // timeoutMs: how long to wait for the remote thread (default 2000 ms).
    static bool Execute( const std::string& code, int timeoutMs = 2000 );

    // Fire-and-forget execution — runs on a detached thread (safe for render loop).
    static void ExecuteAsync( const std::string& code );

    static bool IsReady()       { return s_Ready; }
    static bool IsInitializing(){ return s_Initializing; }

    static void Reset() {
        s_Ready = false; s_LuaState = 0;
        s_fnLoadBuffer = 0; s_fnPcall = 0;
    }

private:
    // ── Shellcode ───────────────────────────────────────────────────────────
    //
    //  Windows x64 ABI: RCX = lpParameter = remote address of ExecArgs
    //
    //  push rbx / push rsi / push rdi
    //  sub  rsp, 58h          ; shadow(32) + 2 extra arg slots (16) + align
    //  mov  rbx, rcx          ; save ExecArgs ptr
    //
    //  ; luaL_loadbuffer(L, buff, sz, name)   —— 4 args, [rsp+20h] = NULL mode (5th)
    //  mov  rcx, [rbx+10h]    ; L
    //  mov  rdx, [rbx+18h]    ; buff
    //  mov  r8d, [rbx+20h]    ; sz  (zero-extends to r8 fine for Lua size_t on x64)
    //  lea  r9,  [rbx+28h]    ; name  (@exec)
    //  xor  rax, rax
    //  mov  [rsp+20h], rax    ; 5th arg slot = NULL (mode for luaL_loadbufferx compat)
    //  call [rbx]             ; fnLoadBuffer
    //
    //  test eax, eax
    //  jnz  .cleanup          ; load failed — skip pcall (+0x1F = 31 bytes ahead)
    //
    //  ; lua_pcall(L, 0, 0, 0)  / lua_pcallk(L, 0, 0, 0, 0, NULL)
    //  mov  rcx, [rbx+10h]
    //  xor  edx, edx          ; nargs = 0
    //  xor  r8d, r8d          ; nresults = 0
    //  xor  r9d, r9d          ; errfunc = 0
    //  xor  rax, rax
    //  mov  [rsp+20h], rax    ; ctx = 0
    //  mov  [rsp+28h], rax    ; k   = NULL
    //  call [rbx+8]           ; fnPcall
    //
    //  mov  [rbx+24h], eax    ; save result to ExecArgs.execResult
    //
    //  .cleanup:
    //  add  rsp, 58h
    //  pop rdi / pop rsi / pop rbx
    //  ret
    //
    //  jnz offset verification:
    //    pcallk body = 4+2+3+3+3+5+5+3 = 28 bytes
    //    mov [rbx+24h],eax  = 3 bytes
    //    total skip = 31 = 0x1F  ✓

    static constexpr uint8_t k_SC[] = {
        0x53,                                    // push rbx
        0x56,                                    // push rsi
        0x57,                                    // push rdi
        0x48, 0x83, 0xEC, 0x58,                 // sub  rsp, 58h
        0x48, 0x89, 0xCB,                        // mov  rbx, rcx

        // luaL_loadbuffer(L, buff, sz, name [, NULL])
        0x48, 0x8B, 0x4B, 0x10,                 // mov  rcx, [rbx+10h]
        0x48, 0x8B, 0x53, 0x18,                 // mov  rdx, [rbx+18h]
        0x44, 0x8B, 0x43, 0x20,                 // mov  r8d, [rbx+20h]
        0x4C, 0x8D, 0x4B, 0x28,                 // lea  r9,  [rbx+28h]
        0x48, 0x31, 0xC0,                        // xor  rax, rax
        0x48, 0x89, 0x44, 0x24, 0x20,           // mov  [rsp+20h], rax
        0xFF, 0x13,                              // call [rbx]

        0x85, 0xC0,                              // test eax, eax
        0x75, 0x1F,                              // jnz  .cleanup (+31)

        // lua_pcall / lua_pcallk
        0x48, 0x8B, 0x4B, 0x10,                 // mov  rcx, [rbx+10h]
        0x31, 0xD2,                              // xor  edx, edx
        0x45, 0x31, 0xC0,                        // xor  r8d, r8d
        0x45, 0x31, 0xC9,                        // xor  r9d, r9d
        0x48, 0x31, 0xC0,                        // xor  rax, rax
        0x48, 0x89, 0x44, 0x24, 0x20,           // mov  [rsp+20h], rax
        0x48, 0x89, 0x44, 0x24, 0x28,           // mov  [rsp+28h], rax
        0xFF, 0x53, 0x08,                        // call [rbx+8]

        0x89, 0x43, 0x24,                        // mov  [rbx+24h], eax  ← execResult

        // .cleanup:
        0x48, 0x83, 0xC4, 0x58,                 // add  rsp, 58h
        0x5F,                                    // pop  rdi
        0x5E,                                    // pop  rsi
        0x5B,                                    // pop  rbx
        0xC3                                     // ret
    };

    // ── ExecArgs struct ─────────────────────────────────────────────────────
    struct alignas(8) ExecArgs {
        uint64_t fnLoadBuffer;    // +0x00  → call [rbx]
        uint64_t fnPcall;         // +0x08  → call [rbx+8]
        uint64_t luaState;        // +0x10  → mov rcx, [rbx+10h]
        uint64_t codePtr;         // +0x18  → mov rdx, [rbx+18h]
        uint32_t codeLen;         // +0x20  → mov r8d, [rbx+20h]
        int32_t  execResult;      // +0x24  ← shellcode writes here; -1 = loadbuffer failed
        char     chunkName[16];   // +0x28  → lea r9, [rbx+28h]  ("@exec")
        //                           +0x38  Lua source string follows in the allocation
    };
    static_assert( offsetof( ExecArgs, chunkName ) == 0x28, "ExecArgs layout mismatch" );
    static_assert( sizeof( ExecArgs ) == 0x38,              "ExecArgs size mismatch"   );

    // ── Internal helpers ────────────────────────────────────────────────────
    struct ModInfo { uintptr_t base = 0; uintptr_t size = 0; };

    static ModInfo   FindLuaModule();
    static uintptr_t GetExport( ModInfo mod, const char* name );
    static std::vector<uintptr_t> ScanLuaStates();
    static bool      IsValidLuaState( uintptr_t addr );
    static bool      TestLuaState( uintptr_t state, const char* testCode );
    static bool      RunShellcode( uintptr_t state, const std::string& code,
                                   int timeoutMs, int* outResult );
};

// ── Static member definitions ────────────────────────────────────────────────
inline bool      LuaExecutor::s_Ready       = false;
inline uintptr_t LuaExecutor::s_LuaState    = 0;
inline uintptr_t LuaExecutor::s_fnLoadBuffer= 0;
inline uintptr_t LuaExecutor::s_fnPcall     = 0;
inline bool      LuaExecutor::s_Initializing= false;

// ── FindLuaModule ────────────────────────────────────────────────────────────
// Tries known FiveM Lua DLL names first, then falls back to a broad
// CreateToolhelp32Snapshot scan for any module whose name contains "lua".
inline LuaExecutor::ModInfo LuaExecutor::FindLuaModule()
{
    // Names FiveM b3258 is known to load
    static const char* k_Names[] = {
        "lua54.dll",
        "citizen-scripting-lua54.dll",
        "citizen-scripting-lua.dll",
        "lua53.dll",
        nullptr
    };

    for ( int i = 0; k_Names[i]; i++ ) {
        uintptr_t modSize = 0;
        uintptr_t base = Mem.GetModuleBaseAddr( Mem.ProcId, k_Names[i], &modSize );
        if ( base ) return { base, modSize };
    }

    // Broad fallback: iterate all modules, look for "lua" in name + verify exports
    HANDLE snap = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Mem.ProcId );
    if ( snap == INVALID_HANDLE_VALUE ) return {};

    MODULEENTRY32W me{};
    me.dwSize = sizeof( me );
    ModInfo result{};

    if ( Module32FirstW( snap, &me ) ) {
        do {
            if ( wcsstr( me.szModule, L"lua" ) || wcsstr( me.szModule, L"Lua" ) ) {
                ModInfo candidate{ (uintptr_t)me.modBaseAddr, me.modBaseSize };
                // Verify it actually exports what we need
                if ( GetExport( candidate, "luaL_loadbuffer"  ) ||
                     GetExport( candidate, "luaL_loadbufferx" ) ) {
                    result = candidate;
                    break;
                }
            }
        } while ( Module32NextW( snap, &me ) );
    }

    CloseHandle( snap );
    return result;
}

// ── GetExport ────────────────────────────────────────────────────────────────
// Reads the PE export table of a remote module via ReadProcessMemory and
// returns the absolute VA of the named export (0 on failure).
inline uintptr_t LuaExecutor::GetExport( ModInfo mod, const char* name )
{
    if ( !mod.base || !mod.size ) return 0;

    // ── DOS header ──────────────────────────────────────────────────────────
    uint8_t dosHdr[64] = {};
    SIZE_T nRead = 0;
    if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)mod.base, dosHdr, 64, &nRead )
         || nRead < 64 )
        return 0;

    uint16_t dosMagic; memcpy( &dosMagic, dosHdr, 2 );
    if ( dosMagic != 0x5A4D ) return 0;  // "MZ"

    uint32_t peOff; memcpy( &peOff, dosHdr + 0x3C, 4 );
    if ( peOff + 0x108 > mod.size ) return 0;

    // ── NT headers (IMAGE_NT_HEADERS64) ─────────────────────────────────────
    //  Offset within NT headers to DataDirectory[0] (export):
    //    4 (PE sig) + 20 (FileHeader) + 112 (DataDirectory offset in OptHdr64)
    //    = 136 = 0x88
    uint8_t ntHdr[0x108] = {};
    if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)(mod.base + peOff), ntHdr, 0x108, &nRead )
         || nRead < 0x108 )
        return 0;

    uint32_t ntSig; memcpy( &ntSig, ntHdr, 4 );
    if ( ntSig != 0x4550 ) return 0;  // "PE"

    uint32_t expRVA, expSize;
    memcpy( &expRVA,  ntHdr + 0x88, 4 );
    memcpy( &expSize, ntHdr + 0x8C, 4 );
    if ( !expRVA || !expSize ) return 0;

    // ── IMAGE_EXPORT_DIRECTORY (40 bytes) ───────────────────────────────────
    //  +20 NumFunctions, +24 NumNames, +28 AddressOfFunctions,
    //  +32 AddressOfNames, +36 AddressOfNameOrdinals
    uint8_t expDir[40] = {};
    if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)(mod.base + expRVA), expDir, 40, &nRead )
         || nRead < 40 )
        return 0;

    uint32_t numFuncs, numNames, rvaFuncs, rvaNames, rvaOrds;
    memcpy( &numFuncs, expDir + 20, 4 );
    memcpy( &numNames, expDir + 24, 4 );
    memcpy( &rvaFuncs, expDir + 28, 4 );
    memcpy( &rvaNames, expDir + 32, 4 );
    memcpy( &rvaOrds,  expDir + 36, 4 );

    if ( !numNames || !rvaNames || !rvaFuncs || !rvaOrds ) return 0;
    if ( numNames > 8192 || numFuncs > 8192 ) return 0;  // sanity cap

    // Read name RVA array, ordinal array, function RVA array
    std::vector<uint32_t> nameRVAs( numNames ), funcRVAs( numFuncs );
    std::vector<uint16_t> ords( numNames );

    if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)(mod.base + rvaNames),
                             nameRVAs.data(), numNames * 4, &nRead ) ) return 0;
    if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)(mod.base + rvaOrds),
                             ords.data(), numNames * 2, &nRead ) ) return 0;
    if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)(mod.base + rvaFuncs),
                             funcRVAs.data(), numFuncs * 4, &nRead ) ) return 0;

    const size_t targetLen = strlen( name );

    for ( uint32_t i = 0; i < numNames; i++ ) {
        char exportName[128] = {};
        SIZE_T r = 0;
        ReadProcessMemory( Mem.ProcHandle, (LPCVOID)(mod.base + nameRVAs[i]),
                           exportName, std::min( (size_t)127u, targetLen + 1 ), &r );
        exportName[127] = 0;
        if ( strcmp( exportName, name ) == 0 && ords[i] < numFuncs )
            return mod.base + funcRVAs[ ords[i] ];
    }
    return 0;
}

// ── ScanLuaStates ────────────────────────────────────────────────────────────
// Walks all MEM_PRIVATE | PAGE_READWRITE pages (heap) in the target process,
// looking for addresses whose byte pattern matches a Lua 5.4 lua_State header.
// Returns up to 20 candidates for further validation.
inline std::vector<uintptr_t> LuaExecutor::ScanLuaStates()
{
    std::vector<uintptr_t> results;
    results.reserve( 16 );

    SYSTEM_INFO si{};
    GetSystemInfo( &si );

    uintptr_t addr    = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;

    static constexpr size_t CHUNK = 65536u;        // 64 KB read at a time
    static constexpr size_t MAX_REGION = 4u * 1024 * 1024; // skip huge regions

    std::vector<uint8_t> buf;
    buf.reserve( CHUNK );

    while ( addr < maxAddr && results.size() < 20 ) {
        MEMORY_BASIC_INFORMATION mbi{};
        if ( !VirtualQueryEx( Mem.ProcHandle, (LPCVOID)addr, &mbi, sizeof(mbi) ) ) {
            addr += 0x1000;
            continue;
        }

        // Only scan private heap RW pages — lua_State lives on the allocator heap
        bool isHeap = ( mbi.State   == MEM_COMMIT   )
                   && ( mbi.Type    == MEM_PRIVATE   )
                   && ( mbi.Protect == PAGE_READWRITE )
                   && ( mbi.RegionSize >= 256 );

        if ( isHeap ) {
            const size_t scanLimit = std::min( mbi.RegionSize, MAX_REGION );
            buf.resize( CHUNK );

            for ( size_t off = 0; off + 64 <= scanLimit && results.size() < 20; off += CHUNK )
            {
                size_t readSize = std::min( CHUNK, scanLimit - off );
                SIZE_T nRead = 0;
                if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)( addr + off ),
                                         buf.data(), readSize, &nRead ) )
                    continue;

                // Scan at 8-byte aligned strides.
                // lua_State.tt (LUA_TTHREAD=8) lives at struct+8, so check buf[i]==8
                // and the candidate starts at buf[i-8].
                // Need at least i-8+56 bytes: guard with  i+48 <= nRead
                for ( size_t i = 8; i + 48 <= nRead; i += 8 )
                {
                    if ( buf[i]   != 0x08 ) continue;  // tt = LUA_TTHREAD
                    if ( buf[i+1] >  0x0F ) continue;  // marked = small GC color
                    if ( buf[i+2] >  1    ) continue;  // status  = 0 (OK) or 1 (YIELD)
                    if ( buf[i+3] >  1    ) continue;  // allowhook

                    uintptr_t candidate = addr + off + ( i - 8 );
                    if ( IsValidLuaState( candidate ) )
                        results.push_back( candidate );
                }
            }
        }

        addr += mbi.RegionSize ? mbi.RegionSize : 0x1000;
    }

    return results;
}

// ── IsValidLuaState ──────────────────────────────────────────────────────────
// Reads 56 bytes from addr and validates the full Lua 5.4 lua_State header:
//   GCObject* next   (+0)  — null or valid heap ptr
//   tt               (+8)  — 8  (LUA_TTHREAD)
//   marked           (+9)  — < 0x10
//   status           (+10) — 0 or 1
//   allowhook        (+11) — 0 or 1
//   StkId top        (+16) — valid ptr
//   global_State* lG (+24) — valid ptr
//   CallInfo* ci     (+32) — valid ptr
//   StkId stack_last (+40) — valid ptr
//   StkId stack      (+48) — valid ptr
//   stack <= top <= stack_last (sanity)
inline bool LuaExecutor::IsValidLuaState( uintptr_t addr )
{
    uint8_t buf[56] = {};
    SIZE_T nRead = 0;
    if ( !ReadProcessMemory( Mem.ProcHandle, (LPCVOID)addr, buf, 56, &nRead )
         || nRead < 56 )
        return false;

    auto isPtr = []( uint64_t v ) -> bool {
        return v >= 0x10000ULL && v <= 0x7FFFFFFFFFFFULL;
    };

    // +0: GCObject* next — may be null (first in GC chain) or valid
    uint64_t gcNext;    memcpy( &gcNext,    buf,      8 );
    if ( gcNext != 0 && !isPtr(gcNext) ) return false;

    // +8–11 already pre-screened by caller, re-check for safety
    if ( buf[8]  != 0x08 ) return false;
    if ( buf[9]  >= 0x10 ) return false;
    if ( buf[10] >  1    ) return false;
    if ( buf[11] >  1    ) return false;

    // +16: top
    uint64_t top;       memcpy( &top,       buf + 16, 8 );
    if ( !isPtr(top) ) return false;

    // +24: l_G
    uint64_t lG;        memcpy( &lG,        buf + 24, 8 );
    if ( !isPtr(lG) ) return false;

    // +32: ci
    uint64_t ci;        memcpy( &ci,        buf + 32, 8 );
    if ( !isPtr(ci) ) return false;

    // +40: stack_last
    uint64_t stackLast; memcpy( &stackLast, buf + 40, 8 );
    if ( !isPtr(stackLast) ) return false;

    // +48: stack
    uint64_t stack;     memcpy( &stack,     buf + 48, 8 );
    if ( !isPtr(stack) ) return false;

    // stack <= top <= stack_last + small slack (one extra frame worth)
    if ( top < stack || top > stackLast + 1024 ) return false;

    return true;
}

// ── RunShellcode ─────────────────────────────────────────────────────────────
// Core injection:  VirtualAllocEx → WriteProcessMemory → CreateRemoteThread
// Same pattern as GiveWeapon.hpp.  Returns true if thread completed within
// timeoutMs; writes pcallk return value to *outResult if non-null.
inline bool LuaExecutor::RunShellcode( uintptr_t state, const std::string& code,
                                        int timeoutMs, int* outResult )
{
    if ( outResult ) *outResult = -1;

    const size_t scSize     = sizeof( k_SC );
    const size_t scAligned  = ( scSize + 15u ) & ~15u;         // pad to 16
    const size_t argsOff    = scAligned;
    const size_t codeOff    = argsOff + sizeof( ExecArgs );     // = argsOff + 0x38
    const size_t codeBytes  = code.size() + 1;                  // include null
    const size_t totalSize  = codeOff + codeBytes;

    LPVOID remote = VirtualAllocEx( Mem.ProcHandle, nullptr, totalSize,
                                    MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if ( !remote ) return false;

    // Build combined buffer (zeroed)
    std::vector<uint8_t> buf( totalSize, 0 );
    memcpy( buf.data(), k_SC, scSize );

    // Fill ExecArgs
    ExecArgs args{};
    args.fnLoadBuffer = s_fnLoadBuffer;
    args.fnPcall      = s_fnPcall;
    args.luaState     = state;
    args.codePtr      = (uintptr_t)remote + codeOff;
    args.codeLen      = (uint32_t)code.size();
    args.execResult   = -1;                   // shellcode writes actual result here
    memcpy( args.chunkName, "@exec", 6 );     // Lua chunk name shown in error messages
    memcpy( buf.data() + argsOff, &args, sizeof(args) );

    // Write Lua source string
    memcpy( buf.data() + codeOff, code.c_str(), codeBytes );

    SIZE_T written = 0;
    if ( !WriteProcessMemory( Mem.ProcHandle, remote, buf.data(), totalSize, &written )
         || written != totalSize )
    {
        VirtualFreeEx( Mem.ProcHandle, remote, 0, MEM_RELEASE );
        return false;
    }

    // Thread entry = start of shellcode; lpParameter = remote ExecArgs
    LPVOID lpParam = (LPVOID)( (uintptr_t)remote + argsOff );

    HANDLE hThread = CreateRemoteThread( Mem.ProcHandle, nullptr, 0,
        (LPTHREAD_START_ROUTINE)remote, lpParam, 0, nullptr );

    if ( !hThread ) {
        VirtualFreeEx( Mem.ProcHandle, remote, 0, MEM_RELEASE );
        return false;
    }

    DWORD waitRes = WaitForSingleObject( hThread, (DWORD)timeoutMs );
    CloseHandle( hThread );

    bool completed = ( waitRes == WAIT_OBJECT_0 );

    // Read back execResult so the caller knows if pcallk returned LUA_OK (0)
    if ( completed && outResult ) {
        ExecArgs back{};
        SIZE_T r = 0;
        ReadProcessMemory( Mem.ProcHandle, lpParam, &back, sizeof(back), &r );
        *outResult = back.execResult;
    }

    VirtualFreeEx( Mem.ProcHandle, remote, 0, MEM_RELEASE );
    return completed;
}

// ── TestLuaState ─────────────────────────────────────────────────────────────
// Runs testCode on the given state; returns true only if thread completed AND
// lua_pcallk returned LUA_OK (0) — meaning the code executed without error.
inline bool LuaExecutor::TestLuaState( uintptr_t state, const char* testCode )
{
    int result = -1;
    bool ok = RunShellcode( state, std::string( testCode ), 1000, &result );
    return ok && ( result == 0 );
}

// ── Init ─────────────────────────────────────────────────────────────────────
inline bool LuaExecutor::Init()
{
    s_Ready      = false;
    s_LuaState   = 0;
    s_fnLoadBuffer = 0;
    s_fnPcall    = 0;

    // 1. Locate the Lua module
    ModInfo luaMod = FindLuaModule();
    if ( !luaMod.base ) return false;

    // 2. Resolve exports — prefer the simpler 4-arg variants (lua_pcall /
    //    luaL_loadbuffer) since the shellcode places NULL/0 in the extra
    //    slots anyway, making both variants ABI-compatible with our stub.
    s_fnLoadBuffer = GetExport( luaMod, "luaL_loadbuffer" );
    if ( !s_fnLoadBuffer )
        s_fnLoadBuffer = GetExport( luaMod, "luaL_loadbufferx" );
    if ( !s_fnLoadBuffer ) return false;

    s_fnPcall = GetExport( luaMod, "lua_pcall" );
    if ( !s_fnPcall )
        s_fnPcall = GetExport( luaMod, "lua_pcallk" );
    if ( !s_fnPcall ) return false;

    // 3. Scan process heap for valid lua_State candidates
    auto candidates = ScanLuaStates();
    if ( candidates.empty() ) return false;

    // 4a. Prefer a state with FiveM native access — test PlayerPedId()
    //     This only resolves in the citizen scripting context.
    for ( uintptr_t st : candidates ) {
        if ( TestLuaState( st, "local _ = PlayerPedId()" ) ) {
            s_LuaState = st;
            s_Ready    = true;
            return true;
        }
    }

    // 4b. Fallback: any state that can execute basic Lua (natives may still
    //     work via other mechanisms; at least we have a working executor).
    for ( uintptr_t st : candidates ) {
        if ( TestLuaState( st, "local x = 1 + 1" ) ) {
            s_LuaState = st;
            s_Ready    = true;
            return true;
        }
    }

    return false;
}

// ── InitAsync ────────────────────────────────────────────────────────────────
inline void LuaExecutor::InitAsync()
{
    if ( s_Ready || s_Initializing ) return;
    s_Initializing = true;
    std::thread( []() {
        Init();
        s_Initializing = false;
    } ).detach();
}

// ── Execute ──────────────────────────────────────────────────────────────────
inline bool LuaExecutor::Execute( const std::string& code, int timeoutMs )
{
    if ( !s_Ready ) return false;
    return RunShellcode( s_LuaState, code, timeoutMs, nullptr );
}

// ── ExecuteAsync ─────────────────────────────────────────────────────────────
inline void LuaExecutor::ExecuteAsync( const std::string& code )
{
    if ( !s_Ready ) return;
    std::string copy = code;
    std::thread( [copy]() {
        RunShellcode( s_LuaState, copy, 2000, nullptr );
    } ).detach();
}

} } // namespace Core::SDK
