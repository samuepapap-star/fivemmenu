#pragma once
#include <Core/ExtensionSettings.hpp>
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/SDK/SDK.hpp>
#include <Core/SDK/TeleportTrace.hpp>
#include "SharkSilentPatch.hpp"
#include <atomic>
#include "SourceOptionLogic.hpp"
#include <chrono>

namespace Core { namespace Features {
class cSharkSilentState {
    uintptr_t site=0,cave=0;
    bool installed=false;
    std::vector<uint8_t> original,patch;
    std::chrono::steady_clock::time_point nextScan{};
    std::atomic<int> state{0};
    std::atomic<int> lastResult{0};
    void Status(int value) {
        if(value!=0)lastResult.store(value);
        if(state.exchange(value)!=value)
            AltosDiagnostics::Log("silent_shark status="+std::to_string(value));
    }
    bool Read(uintptr_t address,void* data,size_t length) {
        SIZE_T count=0;
        return address && ReadProcessMemory(Mem.ProcHandle,reinterpret_cast<LPCVOID>(address),data,length,&count)&&count==length;
    }
    bool Write(uintptr_t address,const void* data,size_t length) {
        SIZE_T count=0;
        return address && WriteProcessMemory(Mem.ProcHandle,reinterpret_cast<LPVOID>(address),data,length,&count)&&count==length;
    }
    uintptr_t FindSite() {
        // Read executable sections without changing their memory protections.
        IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
        if(!Read(Mem.ModBase,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE ||
            dos.e_lfanew<=0 || dos.e_lfanew>0x100000 ||
            !Read(Mem.ModBase+dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE ||
            nt.FileHeader.NumberOfSections>96)return 0;
        const uintptr_t headers=Mem.ModBase+dos.e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
        uintptr_t found=0;
        for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i) {
            IMAGE_SECTION_HEADER section{};
            if(!Read(headers+i*sizeof(section),&section,sizeof(section)) ||
                !(section.Characteristics&IMAGE_SCN_MEM_EXECUTE) ||
                section.VirtualAddress>=Mem.ModBaseSize)continue;
            const size_t length=(std::min)(size_t(section.Misc.VirtualSize),size_t(Mem.ModBaseSize-section.VirtualAddress));
            for(size_t offset=0;offset<length;offset+=4096) {
                const size_t size=(std::min)(size_t(4096+16),length-offset);
                std::vector<uint8_t> bytes(size);SIZE_T count=0;
                ReadProcessMemory(Mem.ProcHandle,reinterpret_cast<LPCVOID>(Mem.ModBase+section.VirtualAddress+offset),bytes.data(),size,&count);
                for(size_t at=0;at+SharkSilentPatch::instructions.size()<=count && at<4096;++at) {
                    if(std::memcmp(bytes.data()+at,SharkSilentPatch::instructions.data(),17))continue;
                    const uintptr_t candidate=Mem.ModBase+section.VirtualAddress+offset+at;
                    if(found && found!=candidate)return 0; // Ambiguous pattern: do not patch.
                    found=candidate;
                }
            }
        }
        return found;
    }
    bool Install(const D3DXVECTOR3& coordinates) {
        if(!site) {
            if(std::chrono::steady_clock::now()<nextScan)return false;
            nextScan=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            site=FindSite();
            if(!site){Status(2);return false;}
        }
        original.resize(17);
        if(!Read(site,original.data(),original.size()) || !SharkSilentPatch::Matches(original)) {
            site=0;Status(2);return false;
        }
        if(!cave)cave=reinterpret_cast<uintptr_t>(VirtualAllocEx(Mem.ProcHandle,nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        if(!cave){Status(3);return false;}
        auto shell=SharkSilentPatch::Shell(site+17);
        const float values[]={coordinates.x,coordinates.y,coordinates.z};
        std::memcpy(shell.data()+40,values,sizeof(values));
        if(!Write(cave,shell.data(),shell.size()) || !FlushInstructionCache(Mem.ProcHandle,reinterpret_cast<LPCVOID>(cave),shell.size())) {
            Status(3);return false;
        }
        patch=SharkSilentPatch::Jump(cave);
        installed=true; // Track partial writes so Restore can recover them.
        if(!Write(site,patch.data(),patch.size())) {Restore();if(!installed)Status(3);return false;}
        FlushInstructionCache(Mem.ProcHandle,reinterpret_cast<LPCVOID>(site),patch.size());
        Status(4);return true;
    }
public:
    const char* StatusText() const {
        int value=state.load();
        if(value==0 && g_Config.SilentAim->Enabled) {
            const int previous=lastResult.load();
            if(previous!=4)value=previous;
        }
        switch(value) {
        case 1:return "Aguardando alvo valido dentro do FOV";
        case 2:return "Instrucoes da mira silenciosa nao identificadas nesta revisao";
        case 3:return "Falha ao preparar a mira silenciosa";
        case 4:return "Mira silenciosa aplicada localmente";
        case 5:return "Falha ao restaurar a mira silenciosa";
        default:return "Desativada ou aguardando tecla/menu fechado";
        }
    }
    void Restore() {
        if(installed) {
            if(!Write(site,original.data(),original.size())) {Status(5);return;}
            FlushInstructionCache(Mem.ProcHandle,reinterpret_cast<LPCVOID>(site),original.size());
            installed=false;
        }
        // Reuse one allocation instead of freeing code another game thread
        // might still be finishing. The process releases it when it exits.
        Status(0);
    }
    void Tick() {
        if(state.load()==5) {Restore();return;}
        const auto cfg=g_Config.SilentAim;
        const bool enabled=cfg->Enabled && !g_MenuInfo.IsOpen && Mem.ProcHandle &&
            GetForegroundWindow()==g_Variables.g_hGameWindow &&
            (!cfg->KeyBind || (GetAsyncKeyState(cfg->KeyBind)&0x8000));
        if(!enabled){Restore();return;}
        auto local=SDK::Pointers::pLocalPlayer;
        if(!local){Restore();Status(1);return;}
        float weaponRange=0.f;
        bool rangeRead=false;
        if(ExtraSettings::SilentAutoDistance&&SourceOptionLogic::KnownWeaponRangeLayout(g_Offsets.CurrentBuild)) {
            auto manager=local->GetWeaponManager();
            auto weapon=manager?manager->GetWeaponInfo():nullptr;
            if(weapon)rangeRead=Read((uintptr_t)weapon+0x28C,&weaponRange,sizeof(weaponRange));
        }
        const int maxDistance=SourceOptionLogic::DistanceLimit(cfg->MaxDistance,
            ExtraSettings::SilentAutoDistance,rangeRead,weaponRange);
        D3DXVECTOR3 target{};
        float closest=1.f;
        bool found=false;
        // SHARK-style selection: apply FOV, visibility and distance per target.
        auto extraTargets=SDK::Game::SnapshotEntities();
        for(const auto& entity:extraTargets) {
            if(!entity.Ped || entity.Ped==local || entity.IsFriend || entity.Health<=101.f ||
                entity.Distance>maxDistance || (cfg->IgnoreNPCs && entity.PedType!=2) ||
                (cfg->OnlyVisible && !entity.Ped->IsVisible()))continue;
            if(ExtraSettings::SilentForceDriver){
                if(!entity.Ped->InVehicle())continue;
                auto car=entity.Ped->GetLastVehicle();if(!car||car->GetDriver()!=entity.Ped)continue;
            }
            const int extraBones[]={0,7,6};
            auto bone=entity.Ped->GetBonePosDefault(extraBones[ImClamp(ExtraSettings::SilentHitbox,0,2)]);
            if(ExtraSettings::SilentPrediction){
                D3DXVECTOR3 velocity{};
                if(Read((uintptr_t)entity.Ped+0x320,&velocity,sizeof(velocity))&&SharkSilentPatch::Sane(velocity)&&
                    std::abs(velocity.x)<150.f&&std::abs(velocity.y)<150.f&&std::abs(velocity.z)<150.f)
                    bone+=velocity*ImClamp(ExtraSettings::PredictionSeconds,0.f,0.3f);
            }
            if(!SharkSilentPatch::Sane(bone) || (bone.x==0 && bone.y==0 && bone.z==0))continue;
            const auto screen=SDK::Game::WorldToScreen(bone);
            if(!std::isfinite(screen.x)||!std::isfinite(screen.y)||screen.x<=0||screen.y<=0||
                screen.x>=g_Variables.g_vGameWindowSize.x || screen.y>=g_Variables.g_vGameWindowSize.y)continue;
            const float distance=std::hypot(screen.x-g_Variables.g_vGameWindowCenter.x,screen.y-g_Variables.g_vGameWindowCenter.y);
            float allowed=float(cfg->FOV);
            if(ExtraSettings::SilentDynamicFov)allowed=ImClamp(float(cfg->FOV)*40.f/(std::max)(entity.Distance,1.f),float((std::min)(ExtraSettings::SilentMinFov,cfg->FOV)),float(cfg->FOV));
            const float score=allowed>0?distance/allowed:2.f;
            if(score<closest){closest=score;target=bone;found=true;}
        }
        if(!found){Restore();Status(1);return;}
        if(cfg->MissChance>0 && Utils::GenRandomInt(0,99)<cfg->MissChance)target.z+=0.4f;
        if(!SharkSilentPatch::Sane(target)){Restore();Status(1);return;}
        if(!installed){Install(target);return;}
        const float values[]={target.x,target.y,target.z};
        if(!Write(cave+40,values,sizeof(values))){Restore();Status(3);}
        else Status(4);
    }
};
inline cSharkSilentState g_SharkSilentState;
}}

