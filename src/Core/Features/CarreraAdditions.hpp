#pragma once
#include <Core/Core.hpp>
#include <array>
#include <cmath>
#include <mutex>

namespace Core { namespace Features { namespace CarreraAdditions {
    inline std::string collisionStatus = "Desativado";
    template<typename T> inline bool Read(uintptr_t address, T& value) {
        SIZE_T bytes=0;
        return address && Mem.ProcHandle &&
            ReadProcessMemory(Mem.ProcHandle,reinterpret_cast<LPCVOID>(address),&value,sizeof(T),&bytes) &&
            bytes==sizeof(T);
    }
    template<typename T> inline bool Write(uintptr_t address,const T& value) {
        SIZE_T bytes=0;
        return address && Mem.ProcHandle &&
            WriteProcessMemory(Mem.ProcHandle,reinterpret_cast<LPVOID>(address),&value,sizeof(T),&bytes) &&
            bytes==sizeof(T);
    }
    inline uintptr_t LocalPlayer() {
        uintptr_t world=0,ped=0;
        return Read(g_Offsets.m_World,world) && world && Read(world+8,ped) ? ped : 0;
    }
    inline bool Position(uintptr_t entity,D3DXVECTOR3& pos) {
        return entity && Read(entity+0x90,pos) &&
            std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z);
    }
    inline uintptr_t CollisionAddress(uintptr_t ped) {
        uintptr_t nav=0,a=0,b=0,c=0,d=0;
        if(!ped || !Read(ped+0x30,nav) || !nav ||
            !Read(nav+0x10,a) || !a || !Read(a+0x20,b) || !b ||
            !Read(b+0x70,c) || !c || !Read(c,d) || !d) return 0;
        return d+0x2C;
    }
    inline void TickCollision(bool force=false) {
        static ULONGLONG lastTick=0;
        const ULONGLONG now=GetTickCount64();
        if(!force && now-lastTick<100) return;
        lastTick=now;
        static uintptr_t owner=0,savedAddress=0;
        static float original=0.f;
        static bool applied=false;
        const uintptr_t ped=LocalPlayer();
        const uintptr_t address=CollisionAddress(ped);
        const bool enabled=g_Config.Player->NoCollisionEnabled;
        if(applied && (ped!=owner || address!=savedAddress)) {
            // Discard obsolete addresses after respawn/unload; never write into an old allocation.
            owner=0;savedAddress=0;applied=false;
        }
        if(!enabled) {
            if(applied) {
                if(Write(savedAddress,original)) {applied=false;collisionStatus="Desativado";}
                else collisionStatus="Falha ao restaurar colisão";
            } else collisionStatus="Desativado";
            return;
        }
        if(!address) {collisionStatus="Personagem ou navegação indisponível";return;}
        float current=0.f;
        if(!Read(address,current) || !std::isfinite(current)) {
            collisionStatus="Não foi possível ler a colisão";return;
        }
        if(!applied) {original=current;savedAddress=address;owner=ped;}
        const float disabled=-0.9999f; // carrera No Collision value
        if(Write(address,disabled)) {applied=true;collisionStatus="Aplicado localmente";}
        else collisionStatus="Não foi possível alterar a colisão";
    }
    inline std::string CopyClothes(uintptr_t selectedPed) {
        const uintptr_t local=LocalPlayer();
        uintptr_t source=0,target=0;
        if(!selectedPed || !local || !Read(selectedPed+0x48,source) || !source ||
            !Read(local+0x48,target) || !target)
            return "Personagem ou roupa indisponível.";
        // Exact component-byte set used by carrera; all reads precede writes.
        constexpr std::array<uintptr_t,9> offsets={0x100,0xF8,0x114,0x108,0xEC,0xF4,0xFC,0x10C,0xAC};
        std::array<BYTE,9> values{},previous{};
        for(size_t i=0;i<offsets.size();++i)
            if(!Read(source+offsets[i],values[i]) || !Read(target+offsets[i],previous[i]))
                return "Não foi possível ler os componentes da roupa.";
        for(size_t i=0;i<offsets.size();++i) {
            if(!Write(target+offsets[i],values[i])) {
                for(size_t rollback=0;rollback<i;++rollback) Write(target+offsets[rollback],previous[rollback]);
                return "Falha na cópia; restauração dos componentes anteriores solicitada.";
            }
        }
        return "Dados de roupa copiados localmente.";
    }
    inline std::string VehiclesToPlayer(uintptr_t selectedPed) {
        D3DXVECTOR3 target{},local{};
        if(!Position(selectedPed,target) || !Position(LocalPlayer(),local))
            return "Posição do jogador indisponível.";
        std::vector<SDK::Game::VehicleStructure> snapshot;
        {
            std::lock_guard<std::mutex> guard(Threads::g_VehicleList.vehicleListMutex);
            snapshot=SDK::Game::VehicleList;
        }
        int updated=0,failed=0;
        for(const auto& item:snapshot) {
            const uintptr_t vehicle=reinterpret_cast<uintptr_t>(item.Pointer);
            D3DXVECTOR3 current{};
            if(!Position(vehicle,current)) continue;
            const D3DXVECTOR3 delta=current-local;
            if(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z>200.f*200.f) continue;
            // Port of the carrera action: move loaded vehicles, not the local player.
            if(Write(vehicle+0x90,target)) ++updated; else ++failed;
        }
        return "Atualizados localmente: "+std::to_string(updated)+". Falhas de escrita: "+std::to_string(failed)+".";
    }
}}}
