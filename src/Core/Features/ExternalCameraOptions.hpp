#pragma once
#include <Core/ExtensionSettings.hpp>
#include <Core/SDK/SDK.hpp>
#include <cmath>
namespace Core { namespace Features { namespace ExternalCameraOptions {
template<class T> bool Read(uintptr_t address, T& value) {
    SIZE_T bytes = 0;
    return address && Mem.ProcHandle && ReadProcessMemory(Mem.ProcHandle, (LPCVOID)address,
        &value, sizeof(value), &bytes) && bytes == sizeof(value);
}
inline bool Write(uintptr_t address, float value) {
    SIZE_T bytes = 0;
    return WriteProcessMemory(Mem.ProcHandle, (LPVOID)address, &value, sizeof(value), &bytes) && bytes == sizeof(value);
}
struct Override {
    uintptr_t address = 0;
    float original = 0, applied = 0;
    void Restore() {
        float current = 0;
        if (address && Read(address, current) && current == applied) Write(address, original);
        address = 0;
    }
    void Apply(uintptr_t next, bool enabled, float value, float minimum, float maximum) {
        if (!enabled || next != address) Restore();
        if (!enabled || !next) return;
        float current = 0;
        if (!Read(next, current) || !std::isfinite(current) || current < minimum || current > maximum) return;
        if (!address) { address = next; original = current; }
        if (Write(next, value)) applied = value;
    }
};
inline Override cameraFov, weaponRange;
inline bool RangeBuildSupported() {
    switch (g_Offsets.CurrentBuild) {
    case 2060: case 2189: case 2372: case 2545: case 2612: case 2699: case 2802: return true;
    default: return false;
    }
}
inline void Tick(bool restore = false) {
    const auto cfg = g_Config.Aimbot;
    uintptr_t camera = 0, data = 0, weapon = 0;
    const bool ready = !restore && g_MenuInfo.IsLogged && Core::SDK::Pointers::pLocalPlayer;
    if (ready && Core::SDK::Pointers::pCamGamePlayDirector &&
        Read(Core::SDK::Pointers::pCamGamePlayDirector + 0x2C0, camera) && camera)
        Read(camera + 0x10, data);
    if (ready && RangeBuildSupported()) {
        const auto manager = Core::SDK::Pointers::pLocalPlayer->GetWeaponManager();
        if (manager) weapon = reinterpret_cast<uintptr_t>(manager->GetWeaponInfo());
    }
    cameraFov.Apply(data ? data + 0x30 : 0, ready && cfg->CameraFovEnabled,
        ImClamp(cfg->CameraFov, 50.f, 160.f), 1.f, 179.f);
    weaponRange.Apply(weapon ? weapon + 0x28C : 0, ready && cfg->WeaponRangeEnabled && RangeBuildSupported(),
        ImClamp(cfg->WeaponRange, 1.f, 1000.f), 0.f, 10000.f);
    if (ready && cfg->Crosshair) {
        auto draw=ImGui::GetBackgroundDrawList();const auto c=g_Variables.g_vGameWindowCenter;
        const float size=ExtraSettings::CrossSize,width=ExtraSettings::CrossWidth;
        const ImU32 col=ImGui::ColorConvertFloat4ToU32(ExtraSettings::CrossColor);
        if(ExtraSettings::CrossType==0||ExtraSettings::CrossType==3){draw->AddLine(ImVec2(c.x-size,c.y),ImVec2(c.x+size,c.y),col,width);draw->AddLine(ImVec2(c.x,c.y-size),ImVec2(c.x,c.y+size),col,width);}
        if(ExtraSettings::CrossType==1||ExtraSettings::CrossType==3)draw->AddCircleFilled(ImVec2(c.x,c.y),width+1,col);
        if(ExtraSettings::CrossType==2)draw->AddCircle(ImVec2(c.x,c.y),size,col,32,width);
    }
}
}}}
