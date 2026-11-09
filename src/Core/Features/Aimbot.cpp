#include "Aimbot.hpp"
#include "ExternalAimMath.hpp"
#include <Core/ExtensionSettings.hpp>
#include <random>
#include "PlayerBones.hpp"

void Core::Features::cAimbot::SetViewAngles(CPed*, D3DXVECTOR3 target)
{
    const auto cfg = g_Config.Aimbot;
    if (!Core::SDK::Pointers::pCamGamePlayDirector) return;
    const uintptr_t camera = Mem.Read<uintptr_t>(Core::SDK::Pointers::pCamGamePlayDirector + 0x2C0);
    if (!camera) return;
    const auto origin = Mem.Read<D3DXVECTOR3>(camera + 0x60);
    D3DXVECTOR3 direction = target - origin;
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) || !std::isfinite(direction.z) ||
        D3DXVec3Length(&direction) < 0.001f) return;
    D3DXVec3Normalize(&direction, &direction);
    for (uintptr_t offset : {uintptr_t(0x40), uintptr_t(0x3D0)}) {
        auto current = Mem.Read<D3DXVECTOR3>(camera + offset);
        auto next = direction;
        if (cfg->AimbotSpeed > 1) {
            next = current + (direction - current) / (7.f * ImClamp(cfg->AimbotSpeed, 1, 100));
            if (D3DXVec3Length(&next) < 0.001f) continue;
            D3DXVec3Normalize(&next, &next);
        }
        Mem.Write<D3DXVECTOR3>(camera + offset, next);
    }
}

void Core::Features::cAimbot::Start()
{
    constexpr unsigned masks[] = {SKEL_Head, SKEL_Pelvis, SKEL_Neck_1, SKEL_R_Clavicle, SKEL_L_Clavicle,
        SKEL_R_UpperArm, SKEL_L_UpperArm, SKEL_L_Forearm, SKEL_R_Forearm, SKEL_L_Hand, SKEL_R_Hand,
        SKEL_R_Thigh, SKEL_R_Calf, SKEL_L_Thigh, SKEL_L_Calf, SKEL_R_Foot, SKEL_L_Foot};
    std::mt19937 random(std::random_device{}());
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
        const auto cfg = g_Config.Aimbot;
        auto local = Core::SDK::Pointers::pLocalPlayer;
        if (!cfg->Enabled || !cfg->KeyBind || g_MenuInfo.IsOpen || !local ||
            GetForegroundWindow() != g_Variables.g_hGameWindow ||
            !(GetAsyncKeyState(cfg->KeyBind) & 0x8000) || (local->InVehicle() && !cfg->InVehicleAim)) continue;
        const auto center = g_Variables.g_vGameWindowCenter;
        const auto size = g_Variables.g_vGameWindowSize;
        CPed* selected = nullptr;
        D3DXVECTOR3 point;
        float best = static_cast<float>(cfg->FOV);
        const int chosen = cfg->BoneMode == 2 ? std::uniform_int_distribution<int>(0,16)(random) : ImClamp(cfg->V7Bone,0,16);
        for (const auto& e : SDK::Game::SnapshotEntities()) {
            auto ped = e.Ped;
            if (!ped || ped == local || e.IsFriend || e.Distance > cfg->MaxDistance ||
                e.Health <= (e.PedType == 2 ? 101.f : 1.f) || (cfg->IgnoreNPCs && e.PedType != 2) ||
                (cfg->HealthFilter && (e.Health - (e.PedType == 2 ? 100.f : 0.f)) < cfg->HealthThreshold) ||
                (cfg->SkipVehicles && ped->InVehicle()) || (cfg->OnlyVisible && !ped->IsVisible())) continue;
            PlayerBones bones;
            const bool hasBones = (cfg->BoneMode == 1 || chosen != 0) && bones.Open(ped);
            for (int i = 0; i < 17; ++i) {
                if (cfg->BoneMode != 1 && i != chosen) continue;
                D3DXVECTOR3 pos;
                if (!i) pos = ped->GetBonePosDefault(0);
                else if (!hasBones || !bones.Position(masks[i], pos)) continue;
                const auto screen = SDK::Game::WorldToScreen(pos);
                if (!ExternalAimMath::Inside(screen.x,screen.y,size.x,size.y)) continue;
                const float radius = std::hypot(screen.x-center.x,screen.y-center.y);
                if (radius < best) { best = radius; selected = ped; point = pos; }
            }
        }
        if (!selected) continue;
        if (cfg->Prediction) point += selected->GetVelocity() * ImClamp(cfg->PredictionTime,0.01f,1.f);
        const float spread = ImClamp(cfg->RandomAngle,0.f,5.f);
        if (spread > 0.f) {
            std::uniform_real_distribution<float> delta(-spread,spread);
            point += D3DXVECTOR3(delta(random),delta(random),delta(random));
        }
        SetViewAngles(selected, point);
    }
}
