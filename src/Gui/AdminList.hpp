#pragma once
#include <Core/ExtensionSettings.hpp>
#include <Core/SDK/SDK.hpp>
#include <cmath>
#include "VisibilityEvidence.hpp"
namespace AdminList {
inline std::vector<std::string> entries;
template<class T> inline bool Read(uintptr_t address,T& value) {
    SIZE_T bytes=0;
    return address && Mem.ProcHandle && ReadProcessMemory(Mem.ProcHandle,
        reinterpret_cast<LPCVOID>(address),&value,sizeof(value),&bytes) && bytes==sizeof(value);
}

// RGB rainbow cycle (ciclo 3s, HSV→RGB)
inline ImColor RgbCycleColor() {
    const float t = static_cast<float>(GetTickCount64() % 3000) / 3000.f;
    const float h = fmodf(t, 1.f);
    const int i = static_cast<int>(h*6.f);
    const float f=h*6.f-i, v=1.f, p=0.f, q=v*(1-f), tv=v*f;
    float r,g,b;
    switch(i%6){
        case 0:r=v; g=tv;b=p; break; case 1:r=q; g=v; b=p; break;
        case 2:r=p; g=v; b=tv;break; case 3:r=p; g=q; b=v; break;
        case 4:r=tv;g=p; b=v; break; default:r=v; g=p; b=q; break;
    }
    return ImColor(r,g,b,1.f);
}

inline void Draw() {
    const auto cfg=g_Config.ESP;
    entries.clear();
    static VisibilityEvidence::Filter filter;
    static DWORD processId=0;
    if(processId!=Mem.ProcId){filter.Clear();processId=Mem.ProcId;}
    // AdminEsp roda independente do toggle principal de ESP
    if (!g_MenuInfo.IsLogged ||
        (!cfg->AdminList && !cfg->AdminEsp && !ExtraSettings::InvisibleWarning)) {
        filter.Clear();
        return;
    }
    const auto now=GetTickCount64();
    filter.Prune(now);
    auto draw=ImGui::GetBackgroundDrawList();

    for (const auto& entity:Core::SDK::Game::EntityList) {
        if (!entity.Ped || entity.Ped==Core::SDK::Pointers::pLocalPlayer || entity.PedType!=2 ||
            entity.Health<=101.f || entity.Distance>cfg->MaxDistance || !std::isfinite(entity.Distance)) continue;
        const auto pos=entity.Pos;
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z) ||
            (pos.x==0 && pos.y==0 && pos.z==0)) continue;
        BYTE visibility=0,alpha=255;
        const uintptr_t ped=reinterpret_cast<uintptr_t>(entity.Ped);
        if (!Read(ped+0x2C,visibility) || !Read(ped+0xAC,alpha)) continue;
        if (!filter.Observe(ped,entity.Id,visibility,alpha,entity.Ped->InVehicle(),now)) continue;
        // Heurística de visibilidade apenas. Não confirma cargo de administrador.
        const std::string name=entity.NetworkInfo.UserName.empty()?"Desconhecido":entity.NetworkInfo.UserName;
        entries.push_back(name+" | ID "+std::to_string(entity.Id)+" | "+std::to_string(int(entity.Distance))+" m");

        if (!cfg->AdminEsp) continue;
        const auto screenFeet=Core::SDK::Game::WorldToScreen(pos);
        if (!Core::SDK::Game::IsOnScreen(screenFeet)) continue;
        D3DXVECTOR3 headPos=pos; headPos.z+=1.8f;
        const auto screenHead=Core::SDK::Game::WorldToScreen(headPos);
        if (!Core::SDK::Game::IsOnScreen(screenHead)) continue;

        // Nome RGB acima do ped
        const ImColor rgb=RgbCycleColor();
        const ImVec2 namePos(screenHead.x, screenHead.y-14.f);
        draw->AddText(ImGui::GetFont(),13.f,ImVec2(namePos.x+1.f,namePos.y+1.f),IM_COL32(0,0,0,200),name.c_str());
        draw->AddText(ImGui::GetFont(),13.f,namePos,rgb,name.c_str());
    }
}
inline void RenderEntries() {
    if(entries.empty())ImGui::TextWrapped("Nenhum indicador nos jogadores carregados.");
    for(const auto& row:entries)ImGui::TextWrapped("%s",row.c_str());
}
}

