#pragma once
#include <Core/Features/CarreraAdditions.hpp>
#include <Core/Threads/VehicleList.hpp>
#include <Bridge/Features.hpp>
#include <atomic>
#include <cmath>

// Offsets locais — espelham off:: de AcBypass.hpp sem re-incluir o header
// (re-incluir causaria LNK2005 por funcoes nao-inline duplicadas)
namespace GrabLaunchOff {
    constexpr uintptr_t kVehPos      = 0x30;   // off::kVehPos
    constexpr uintptr_t kVehVelocity = 0x7E4;  // off::kVehVelocity
}

// GrabLaunch — Agarrar & Arremessar veiculo
// Aproxime-se do veiculo, clique "Agarrar Veiculo", depois "Arremessar!"
// NOTA: offset do heading do ped (0x280) marcado como FILL — verificar no Cheat Engine:
//       girar personagem e encontrar o float que muda entre 0 e 360.
//       Se diferir de 0x280, corrigir nas 2 ocorrencias abaixo (FindVehicleAhead e Throw).

namespace GrabLaunch {

    constexpr float k_GrabRange   = 12.0f;
    constexpr float k_FovCos      = 0.65f;
    constexpr float k_HoldHeight  = 2.4f;
    constexpr float k_LaunchSpeed = 160.0f;
    constexpr float k_UpBias      = 0.40f;

    constexpr const char* k_AnimDict = "anim@mp_player_intcelebrationmale@raise_the_roof";
    constexpr const char* k_AnimName = "raise_the_roof_01_player";

    enum class Phase { Idle, Holding, Throwing };
    static std::atomic<Phase> g_Phase { Phase::Idle };
    static uintptr_t g_HeldVeh = 0;

    // -------------------------------------------------------------------------
    // Helpers de vetor
    // -------------------------------------------------------------------------
    inline float Vec3Len(const D3DXVECTOR3& v) {
        return std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    inline D3DXVECTOR3 Vec3Norm(const D3DXVECTOR3& v) {
        const float len = Vec3Len(v);
        if (len < 1e-6f) return D3DXVECTOR3(0.f, 0.f, 0.f);
        return D3DXVECTOR3(v.x / len, v.y / len, v.z / len);
    }
    inline float Vec3Dot(const D3DXVECTOR3& a, const D3DXVECTOR3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    inline D3DXVECTOR3 HeadingToForward(float deg) {
        const float rad = deg * (3.14159265f / 180.f);
        return D3DXVECTOR3(std::sinf(rad), std::cosf(rad), 0.f);
    }

    // -------------------------------------------------------------------------
    // FindVehicleAhead
    // -------------------------------------------------------------------------
    inline uintptr_t FindVehicleAhead() {
        using namespace Core::Features::CarreraAdditions;

        const uintptr_t ped = LocalPlayer();
        if (!ped) return 0;

        D3DXVECTOR3 pedPos{};
        if (!Read(ped + 0x90, pedPos)) return 0;

        // FILL: offset do heading — verificar no Cheat Engine (0x280 e candidato)
        float headingDeg = 0.f;
        Read(ped + 0x280, headingDeg);

        const D3DXVECTOR3 forward = HeadingToForward(headingDeg);

        std::vector<Core::SDK::Game::VehicleStructure> snapshot;
        {
            std::lock_guard<std::mutex> guard(Core::Threads::g_VehicleList.vehicleListMutex);
            snapshot = Core::SDK::Game::VehicleList;
        }

        uintptr_t best = 0;
        float bestDist = k_GrabRange;

        for (const auto& item : snapshot) {
            const uintptr_t veh = reinterpret_cast<uintptr_t>(item.Pointer);
            if (!veh) continue;

            const D3DXVECTOR3 delta = item.Pos - pedPos;
            const float dist = Vec3Len(delta);
            if (dist > k_GrabRange || dist < 0.5f) continue;

            const D3DXVECTOR3 dir = Vec3Norm(delta);
            const float dot = Vec3Dot(
                D3DXVECTOR3(forward.x, forward.y, 0.f),
                D3DXVECTOR3(dir.x,     dir.y,     0.f));
            if (dot < k_FovCos) continue;

            if (dist < bestDist) { bestDist = dist; best = veh; }
        }
        return best;
    }

    // -------------------------------------------------------------------------
    // Grab
    // -------------------------------------------------------------------------
    inline void Grab() {
        if (g_Phase.load() != Phase::Idle) return;
        const uintptr_t veh = FindVehicleAhead();
        if (!veh) return;
        g_HeldVeh = veh;
        g_Phase.store(Phase::Holding);
        BridgeFeatures::Animate(k_AnimDict, k_AnimName, true);
    }

    // -------------------------------------------------------------------------
    // Throw
    // -------------------------------------------------------------------------
    inline void Throw() {
        if (g_Phase.load() != Phase::Holding) return;
        if (!g_HeldVeh) { g_Phase.store(Phase::Idle); return; }

        using namespace Core::Features::CarreraAdditions;

        const uintptr_t ped = LocalPlayer();
        if (ped) {
            // FILL: offset do heading — verificar no Cheat Engine (0x280 e candidato)
            float headingDeg = 0.f;
            Read(ped + 0x280, headingDeg);

            const D3DXVECTOR3 fwd = HeadingToForward(headingDeg);
            const D3DXVECTOR3 velocity(
                fwd.x * k_LaunchSpeed,
                fwd.y * k_LaunchSpeed,
                k_LaunchSpeed * k_UpBias
            );
            Write(g_HeldVeh + GrabLaunchOff::kVehVelocity, velocity);
        }

        BridgeFeatures::StopAnimation();
        g_Phase.store(Phase::Idle);
        g_HeldVeh = 0;
    }

    // -------------------------------------------------------------------------
    // Tick — trava o carro acima do ped enquanto Holding
    // -------------------------------------------------------------------------
    inline void Tick() {
        if (g_Phase.load() != Phase::Holding) return;
        if (!g_HeldVeh) { g_Phase.store(Phase::Idle); return; }

        using namespace Core::Features::CarreraAdditions;

        const uintptr_t ped = LocalPlayer();
        if (!ped) { Throw(); return; }

        D3DXVECTOR3 pedPos{};
        if (!Read(ped + 0x90, pedPos)) return;

        const D3DXVECTOR3 holdPos(pedPos.x, pedPos.y, pedPos.z + k_HoldHeight);

        // Posicao visual
        Write(g_HeldVeh + static_cast<uintptr_t>(0x90u), holdPos);

        // Posicao de navegacao
        uintptr_t nav = 0;
        if (Read(g_HeldVeh + GrabLaunchOff::kVehPos, nav) && nav)
            Write(nav + static_cast<uintptr_t>(0x50), holdPos);

        // Zerar velocidade
        const D3DXVECTOR3 zero(0.f, 0.f, 0.f);
        Write(g_HeldVeh + GrabLaunchOff::kVehVelocity, zero);
    }

    // -------------------------------------------------------------------------
    // State helpers
    // -------------------------------------------------------------------------
    inline bool IsIdle()    { return g_Phase.load() == Phase::Idle; }
    inline bool IsHolding() { return g_Phase.load() == Phase::Holding; }

} // namespace GrabLaunch
