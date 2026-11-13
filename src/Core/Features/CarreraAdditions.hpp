#pragma once
#include <Core/Core.hpp>
#include <Core/SDK/TeleportTrace.hpp>
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
    inline std::string DestroyCurrentVehicle() {
        // Quarntime's negative-condition action, adapted to this SDK's engine field.
        // Operates only on the vehicle the local ped is currently occupying.
        const uintptr_t ped = LocalPlayer();
        if (!ped || !reinterpret_cast<CPed*>(ped)->InVehicle())
            return "Entre em um veículo para usar esta função.";
        uintptr_t vehicle = 0;
        if (!Read(ped + g_Offsets.m_LastVehicle, vehicle) || !vehicle)
            return "Veículo atual indisponível.";
        if (!g_Offsets.m_VehicleEngineHealth)
            return "Campo de condição do motor indisponível nesta build.";
        BYTE invincible = 0;
        if (!Read(vehicle + 0x189, invincible))
            return "Não foi possível verificar a proteção do veículo.";
        if (g_Config.Player->VehicleGodMode || invincible)
            return "Desative a invencibilidade do veículo antes de danificá-lo.";
        float engine = 0.f;
        const uintptr_t condition = vehicle + g_Offsets.m_VehicleEngineHealth;
        if (!Read(condition, engine) || !std::isfinite(engine))
            return "Não foi possível ler a condição do motor.";
        uintptr_t currentVehicle = 0;
        if (LocalPlayer() != ped || !reinterpret_cast<CPed*>(ped)->InVehicle() ||
            !Read(ped + g_Offsets.m_LastVehicle, currentVehicle) || currentVehicle != vehicle)
            return "O veículo atual mudou. Tente novamente.";
        if (!Write(condition, -4000.f))
            return "Falha ao escrever o dano no motor.";
        return "Valor de dano enviado ao motor. Efeito no jogo não confirmado.";
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
    inline std::string VehiclesToPlayer(uintptr_t selectedPed, bool rain=false) {
        AltosDiagnostics::Log("action=veiculos_para_jogador click");
        D3DXVECTOR3 target{},local{};
        if(!Position(selectedPed,target) || !Position(LocalPlayer(),local)) {
            AltosDiagnostics::Log("action=veiculos_para_jogador invalid_player_position");
            return "Posição do jogador indisponível.";
        }
        std::vector<SDK::Game::VehicleStructure> snapshot;
        {
            std::lock_guard<std::mutex> guard(Threads::g_VehicleList.vehicleListMutex);
            snapshot=SDK::Game::VehicleList;
        }
        int updated=0,failed=0,invalid=0,outside=0,nearby=0;
        for(const auto& item:snapshot) {
            const uintptr_t vehicle=reinterpret_cast<uintptr_t>(item.Pointer);
            D3DXVECTOR3 current{};
            if(!Position(vehicle,current)) {++invalid;continue;}
            const D3DXVECTOR3 delta=current-local;
            if(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z>200.f*200.f) {++outside;continue;}
            ++nearby;
            D3DXVECTOR3 destination=target;
            if(rain){float angle=nearby*2.399963f;destination.x+=std::cos(angle)*5.f;destination.y+=std::sin(angle)*5.f;destination.z+=15.f;}
            uintptr_t navigation=0;
            if(!Read(vehicle+0x30,navigation) || !navigation) {++failed;continue;}
            // Same dual-transform update used by SHARK CVehicle::SetPos.
            AltosDiagnostics::TeleportTrace trace(Mem.ProcHandle,vehicle,destination,
                g_Offsets.CurrentBuild,"veiculo",nearby==1);
            const bool visual=Write(vehicle+0x90,destination);
            const bool nav=Write(navigation+0x50,destination);
            trace.Results(visual,nav);
            if(visual && nav)++updated;else ++failed;
        }
        AltosDiagnostics::Log("action=veiculos_para_jogador loaded="+std::to_string(snapshot.size())+
            " nearby="+std::to_string(nearby)+" invalid="+std::to_string(invalid)+
            " outside="+std::to_string(outside)+" updated="+std::to_string(updated)+
            " failed="+std::to_string(failed));
        if(snapshot.empty())return "Nenhum veículo carregado na lista do cliente.";
        if(!nearby)return "Nenhum veículo legível a até 200 m de você. Carregados: "+
            std::to_string(snapshot.size())+". Ilegíveis: "+std::to_string(invalid)+".";
        return "Escritas completas em "+std::to_string(updated)+" de "+std::to_string(nearby)+
            " veículos próximos. Falhas: "+std::to_string(failed)+".";
    }
    // Destroi todos os veiculos no raio de 20 metros via engine health = -4000.
    // Nao usa bridge/natives — opera direto via RPM/WPM na lista local.
    // Ignora o veiculo atual do player e veiculos com GodMode ativo.
    inline std::string DeleteVehiclesRadius() {
        if (!g_Offsets.m_VehicleEngineHealth)
            return "Campo de saúde do motor indisponível nesta build.";
        const uintptr_t ped = LocalPlayer();
        if (!ped) return "Player indisponível.";
        // Ler veiculo atual do ped para ignorar
        uintptr_t currentVeh = 0;
        Read(ped + g_Offsets.m_LastVehicle, currentVeh);
        // Snapshot thread-safe da lista
        const auto snapshot = Core::SDK::Game::SnapshotVehicles();
        int deleted = 0, skipped = 0, failed = 0;
        for (const auto& v : snapshot) {
            if (!v.Pointer || !std::isfinite(v.Dist)) continue;
            if (v.Dist > 20.f) continue; // fora do raio
            const uintptr_t vehAddr = reinterpret_cast<uintptr_t>(v.Pointer);
            if (vehAddr == currentVeh) { ++skipped; continue; } // veiculo atual, ignora
            // Verificar GodMode (invincible flag +0x189)
            BYTE invincible = 0;
            Read(vehAddr + 0x189, invincible);
            if (invincible) { ++skipped; continue; }
            // Escrever engine health = -4000 para destruir
            const uintptr_t condition = vehAddr + g_Offsets.m_VehicleEngineHealth;
            if (Write(condition, -4000.f)) ++deleted; else ++failed;
        }
        if (deleted == 0 && skipped == 0 && failed == 0)
            return "Nenhum veículo encontrado no raio de 20 m.";
        std::string result = "Destruídos: " + std::to_string(deleted);
        if (skipped) result += " | Ignorados: " + std::to_string(skipped);
        if (failed)  result += " | Falhas: " + std::to_string(failed);
        return result;
    }
}}}
