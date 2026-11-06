#pragma once
#include <Core/SDK/SDK.hpp>
#include <chrono>

namespace Core { namespace Features { namespace SelfGodMode {
constexpr DWORD protectionBit=DWORD(1)<<9; // Same player flag as CPed::SetGodMode.
inline uintptr_t owner=0;
inline DWORD ownerProcess=0;
inline bool originalProtection=false;
inline std::string status="Desativado";
template<class T> inline bool Read(uintptr_t at,T& value) {
    SIZE_T transferred=0;
    return at>=0x10000 && Mem.ProcHandle &&
        ReadProcessMemory(Mem.ProcHandle,reinterpret_cast<LPCVOID>(at),&value,sizeof(value),&transferred) &&
        transferred==sizeof(value);
}
inline bool Protection(uintptr_t ped,bool enabled) {
    DWORD flags=0;
    if(!Read(ped+0x188,flags))return false;
    const DWORD updated=enabled?(flags|protectionBit):(flags&~protectionBit);
    if(updated==flags)return true;
    SIZE_T transferred=0;
    return WriteProcessMemory(Mem.ProcHandle,reinterpret_cast<LPVOID>(ped+0x188),&updated,sizeof(updated),&transferred) &&
        transferred==sizeof(updated);
}
inline void Tick(bool forceRestore=false) {
    uintptr_t world=0,ped=0;
    const DWORD process=Mem.ProcHandle?GetProcessId(Mem.ProcHandle):0;
    if(owner && process!=ownerProcess) {owner=0;ownerProcess=0;}
    if(!Read(g_Offsets.m_World,world) || !Read(world+8,ped)) {
        status="Aguardando personagem";
        return;
    }
    // Never write to the old entity after respawn, disconnect or process change.
    if(owner && (ped!=owner || process!=ownerProcess)) {owner=0;ownerProcess=0;}
    if(forceRestore || !g_Config.Player->EnableGodMode || !g_MenuInfo.IsLogged) {
        if(owner && !Protection(owner,originalProtection)) {
            status="Falha ao restaurar a protecao anterior";
            return;
        }
        owner=0;ownerProcess=0;status="Desativado";return;
    }
    if(!ped || !process){status="Aguardando personagem";return;}
    using Clock=std::chrono::steady_clock;
    static Clock::time_point next{};
    if(owner && Clock::now()<next)return;
    next=Clock::now()+std::chrono::milliseconds(100);
    if(!owner) {
        DWORD flags=0;
        if(!Read(ped+0x188,flags)){status="Nao foi possivel ler a protecao";return;}
        owner=ped;ownerProcess=process;originalProtection=(flags&protectionBit)!=0;
    }
    status=Protection(ped,true)?"Protecao local aplicada":"Falha ao aplicar a protecao";
}
}}}
