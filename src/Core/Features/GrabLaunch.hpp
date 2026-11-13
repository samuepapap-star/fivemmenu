#pragma once
#include <Core/Features/CarreraAdditions.hpp>
#include <Core/Threads/VehicleList.hpp>
#include <Bridge/Features.hpp>
#include <atomic>
#include <cmath>

// GrabLaunch — Agarrar & Arremessar veiculo
// Aproxime-se do veiculo, clique "Agarrar Veiculo", depois "Arremessar!"
// NOTA: offset do heading do ped (0x280) marcado como FILL — verificar no Cheat Engine:
//       girar personagem e encontrar o float que muda entre 0 e 360.
//       Se diferir de 0x280, corrigir nas 2 ocorrencias abaixo (FindVehicleAhead e Throw).

namespace GrabLaunch {

    constexpr float k_GrabRange   = 12.0f;   // metros maximos para agarrar
    constexpr float k_FovCos      = 0.65f;   // cosseno do cone de mira (~49 graus)
    constexpr float k_HoldHeight  = 2.4f;    // metros acima do ped para travar o carro
    constexpr float k_LaunchSpeed = 160.0f;  // m/s do arremesso
    constexpr float k_UpBias      = 0.40f;   // componente vertical do arremesso

    // Dicionario/animacao raise_the_roof via bridge
    constexpr const char* k_AnimDict = "anim@mp_player_intcelebrationmale@raise_the_roof";
    constexpr const char* k_AnimName = "raise_the_roof_01_player";

    enum class Phase { Idle, Holding, Throwing };
    static std::atomic<Phase> g_Phase { Phase::Idle };
    static uintptr_t g_HeldVeh = 0;  // ponteiro raw do veiculo segurado

    // -------------------------------------------------------------------------
    // Helpers de vetor usando D3DXVECTOR3 (padrao do projeto)
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

    // Converte heading em graus (0-360) para vetor de direcao no plano XY
    inline D3DXVECTOR3 HeadingToForward(float deg) {
        const float rad = deg * (3.14159265f / 180.f);
        return D3DXVECTOR3(std::sinf(rad), std::cosf(rad), 0.f);
    }

    // -------------------------------------------------------------------------
    // FindVehicleAhead — busca o veiculo mais proximo dentro do cone de mira
    // Usa SDK::Game::VehicleList (thread-populada, padrao do projeto)
    // -------------------------------------------------------------------------
    inline uintptr_t FindVehicleAhead() {
        using namespace Core::Features::CarreraAdditions;

        const uintptr_t ped = LocalPlayer();
        if (!ped) return 0;

        D3DXVECTOR3 pedPos{};
        if (!Read(ped + 0x90, pedPos)) return 0;

        // FILL: offset do heading — verificar no Cheat Engine (0x280 e candidato)
        float headingDeg = 0.f;
        Read(ped + 0x280, headingDeg); // <<< FILL: confirmar offset do heading

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

            // Checar se esta dentro do cone de FOV
            const D3DXVECTOR3 dir = Vec3Norm(delta);
            const float dot = Vec3Dot(D3DXVECTOR3(forward.x, forward.y, 0.f),
                                       D3DXVECTOR3(dir.x, dir.y, 0.f));
            if (dot < k_FovCos) continue;

            if (dist < bestDist) {
                bestDist = dist;
                best = veh;
            }
        }

        return best;
    }

    // -------------------------------------------------------------------------
    // Grab — encontra veiculo, inicia animacao, entra no estado Holding
    // -------------------------------------------------------------------------
    inline void Grab() {
        if (g_Phase.load() != Phase::Idle) return;

        const uintptr_t veh = FindVehicleAhead();
        if (!veh) return;

        g_HeldVeh = veh;
        g_Phase.store(Phase::Holding);

        // Disparar animacao raise_the_roof via bridge (loop = true)
        BridgeFeatures::Animate(k_AnimDict, k_AnimName, true);
    }

    // -------------------------------------------------------------------------
    // Throw — le o heading do ped, calcula velocidade e escreve no veiculo
    // -------------------------------------------------------------------------
    inline void Throw() {
        if (g_Phase.load() != Phase::Holding) return;
        if (!g_HeldVeh) { g_Phase.store(Phase::Idle); return; }

        using namespace Core::Features::CarreraAdditions;

        const uintptr_t ped = LocalPlayer();
        if (ped) {
            // FILL: offset do heading — verificar no Cheat Engine (0x280 e candidato)
            float headingDeg = 0.f;
            Read(ped + 0x280, headingDeg); // <<< FILL: confirmar offset do heading

            const D3DXVECTOR3 fwd = HeadingToForward(headingDeg);
            const D3DXVECTOR3 velocity(
                fwd.x * k_LaunchSpeed,
                fwd.y * k_LaunchSpeed,
                k_LaunchSpeed * k_UpBias
            );

            // Escrever velocidade no veiculo usando offset do namespace off::
            Write(g_HeldVeh + off::kVehVelocity, velocity);
        }

        // Parar animacao
        BridgeFeatures::StopAnimation();

        g_Phase.store(Phase::Idle);
        g_HeldVeh = 0;
    }

    // -------------------------------------------------------------------------
    // Tick — chamado todo frame; enquanto Holding, trava o carro acima do ped
    // -------------------------------------------------------------------------
    inline void Tick() {
        if (g_Phase.load() != Phase::Holding) return;
        if (!g_HeldVeh) { g_Phase.store(Phase::Idle); return; }

        using namespace Core::Features::CarreraAdditions;

        const uintptr_t ped = LocalPlayer();
        if (!ped) { Throw(); return; }

        D3DXVECTOR3 pedPos{};
        if (!Read(ped + 0x90, pedPos)) return;

        // Posicao alvo: diretamente acima do ped
        const D3DXVECTOR3 holdPos(pedPos.x, pedPos.y, pedPos.z + k_HoldHeight);

        // Escrever posicao visual e de navegacao (mesmo padrao de VehiclesToPlayer)
        uintptr_t nav = 0;
        Write(g_HeldVeh + 0x90, holdPos);
        if (Read(g_HeldVeh + off::kVehPos, nav) && nav)
            Write(nav + 0x50, holdPos);

        // Zerar velocidade para o carro ficar parado no ar
        const D3DXVECTOR3 zero(0.f, 0.f, 0.f);
        Write(g_HeldVeh + off::kVehVelocity, zero);
    }

    // -------------------------------------------------------------------------
    // State helpers
    // -------------------------------------------------------------------------
    inline bool IsIdle()    { return g_Phase.load() == Phase::Idle; }
    inline bool IsHolding() { return g_Phase.load() == Phase::Holding; }

} // namespace GrabLaunch
