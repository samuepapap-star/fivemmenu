#include "Aimbot.hpp"
#include "ExternalAimMath.hpp"
#include <Core/ExtensionSettings.hpp>
#include <random>

// External adaptation of the supplied source's mouse and stepped camera modes.
// SilentAim and TriggerBot keep their existing, independent implementations.
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
    const float distance = D3DXVec3Length(&direction);
    D3DXVec3Normalize(&direction, &direction);
    float sx = static_cast<float>(cfg->SeparateSmooth ? cfg->SmoothHorizontal : cfg->AimbotSpeed);
    float sy = static_cast<float>(cfg->SeparateSmooth ? cfg->SmoothVertical : cfg->AimbotSpeed);
    sx = ImClamp(sx, 1.f, 100.f); sy = ImClamp(sy, 1.f, 100.f);
    if (cfg->LegitMode) { sx *= 3.8f; sy *= 3.8f; }
    for (const uintptr_t offset : {uintptr_t(0x40), uintptr_t(0x3D0)}) {
        const auto angles = Mem.Read<D3DXVECTOR3>(camera + offset);
        D3DXVECTOR3 next(ExternalAimMath::Step(angles.x, direction.x, sx),
            ExternalAimMath::Step(angles.y, direction.y, sx),
            ExternalAimMath::Step(angles.z, direction.z, sy));
        if (ExtraSettings::AimHumanization) {
            const float scale = ImClamp(distance / 5.f, 0.1f, 1.f);
            next.x = angles.x + (direction.x - angles.x) * scale / sx;
            next.y = angles.y + (direction.y - angles.y) * scale / sx;
            next.z = angles.z + (direction.z - angles.z) * scale / sy;
        }
        Mem.Write<D3DXVECTOR3>(camera + offset, next);
    }
}

void Core::Features::cAimbot::Start()
{
    while (true) {
        // Always yield, including when no target exists.
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
        const auto cfg = g_Config.Aimbot;
        if (!cfg->Enabled || g_MenuInfo.IsOpen || !Core::SDK::Pointers::pLocalPlayer ||
            GetForegroundWindow() != g_Variables.g_hGameWindow ||
            (cfg->KeyBind && !(GetAsyncKeyState(cfg->KeyBind) & 0x8000))) continue;
        CPed* selected = nullptr;
        D3DXVECTOR3 target;
        D3DXVECTOR2 screen;
        float best = FLT_MAX;
        const auto center = g_Variables.g_vGameWindowCenter;
        const auto size = g_Variables.g_vGameWindowSize;
        for (const auto& entity : Core::SDK::Game::EntityList) {
            CPed* ped = entity.Ped;
            if (!ped || ped == Core::SDK::Pointers::pLocalPlayer || entity.IsFriend ||
                entity.Health <= (entity.PedType == 2 ? 101.f : 1.f) || entity.Distance > cfg->MaxDistance) continue;
            if ((cfg->TargetType == 1 && entity.PedType != 2) ||
                (cfg->TargetType == 2 && entity.PedType == 2)) continue;
            if (cfg->OnlyVisible && !ped->IsVisible()) continue;
            auto point = ped->GetBonePosDefault(ExternalAimMath::Bone(cfg->Hitbox));
            if (cfg->Prediction) {
                const auto velocity = ped->GetVelocity();
                if (std::isfinite(velocity.x) && std::isfinite(velocity.y) && std::isfinite(velocity.z))
                    point += velocity * ExtraSettings::AimLeadSeconds;
            }
            const auto projected = Core::SDK::Game::WorldToScreen(point);
            if (!ExternalAimMath::Inside(projected.x, projected.y, size.x, size.y)) continue;
            const float radius = std::hypot(projected.x - center.x, projected.y - center.y);
            if (radius > cfg->FOV) continue;
            const float rank = cfg->SelectionMode == 0 ? entity.Distance : radius;
            if (rank < best) { best = rank; selected = ped; target = point; screen = projected; }
        }
        if (!selected) continue;
        if (cfg->AimMode == 1) {
            SetViewAngles(selected, target);
            if (ExtraSettings::AimHumanization) {
                static thread_local std::mt19937 generator{std::random_device{}()};
                std::uniform_int_distribution<int> delay(0, 6);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay(generator)));
            }
            continue;
        }
        float sx = float(cfg->SeparateSmooth ? cfg->SmoothHorizontal : cfg->AimbotSpeed);
        float sy = float(cfg->SeparateSmooth ? cfg->SmoothVertical : cfg->AimbotSpeed);
        sx = ImClamp(sx, 1.f, 100.f); sy = ImClamp(sy, 1.f, 100.f);
        if (cfg->LegitMode) { sx *= 3.8f; sy *= 3.8f; }
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dx = LONG(std::lround((screen.x - center.x) / (sx + 1.5f)));
        input.mi.dy = LONG(std::lround((screen.y - center.y) / (sy + 1.5f)));
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        if (input.mi.dx || input.mi.dy) SendInput(1, &input, sizeof(input));
    }
}
